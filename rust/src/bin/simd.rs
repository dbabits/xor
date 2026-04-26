// Portable SIMD via runtime dispatch.
//
// Each hot loop is wrapped with `#[multiversion::multiversion(targets = "simd")]`,
// which compiles the function once per common SIMD ISA (x86_64+sse2/+avx/+avx2/
// +avx512f, aarch64+neon, wasm32+simd128, …) and selects the best at runtime via
// an atomic-cached function pointer. The bodies are written so LLVM's
// autovectorizer recognises them for every target — fixed-stride chunks for XOR,
// branchless arithmetic for the base16 paths.

use std::io::{self, BufWriter, Read, Write};
use std::process;

const CHUNK: usize = 65536;

enum Op {
    Xor,
    Encode,
    Decode,
}

// ---- XOR -------------------------------------------------------------------

// Inner loop: 16-byte slabs XOR'd with a 16-byte tile. Trivially autovectorised
// to PXOR / VPXOR / EOR — the inner loop is fully unrolled by LLVM and widened
// to whatever the target SIMD width allows.
#[multiversion::multiversion(targets = "simd")]
fn xor_tiled_16(buf: &mut [u8], tile: &[u8; 16]) {
    for chunk in buf.chunks_exact_mut(16) {
        for j in 0..16 {
            chunk[j] ^= tile[j];
        }
    }
}

// Build a 16-byte repeating tile from a key. Returns Some only if `klen`
// divides 16 (klen ∈ {1, 2, 4, 8, 16}); otherwise the caller falls back to
// the scalar cycling loop.
fn build_tile(key: &[u8]) -> Option<[u8; 16]> {
    let klen = key.len();
    if klen == 0 || 16 % klen != 0 {
        return None;
    }
    let mut tile = [0u8; 16];
    for i in 0..16 {
        tile[i] = key[i % klen];
    }
    Some(tile)
}

fn xor_buf(buf: &mut [u8], key: &[u8], pos: &mut usize) {
    let klen = key.len();
    if let Some(tile) = build_tile(key) {
        let head_len = buf.len() & !15;
        xor_tiled_16(&mut buf[..head_len], &tile);
        // Tail: at most 15 bytes; klen divides 16 so the tile is correct here too.
        for (i, b) in buf[head_len..].iter_mut().enumerate() {
            *b ^= tile[i];
        }
        // klen divides 16, so a full pass leaves key position unchanged;
        // the tail of length `r < 16` advances pos by `r mod klen`.
        *pos = (*pos + buf.len()) % klen;
    } else {
        // Generic cycling key — not autovectorised, but rare in practice.
        let mut p = *pos;
        for b in buf.iter_mut() {
            *b ^= key[p];
            p += 1;
            if p == klen {
                p = 0;
            }
        }
        *pos = p;
    }
}

// ---- base16 encode ---------------------------------------------------------

// Branchless nibble → ASCII hex char. Pure u8 arithmetic with one signed shift,
// which is exactly what the autovectoriser wants:
//   n in 0..=9   → b'0' + n
//   n in 10..=15 → b'a' + n - 10
// Mask = 0xff if n >= 10 else 0x00 (computed via signed shift of `9 - n`).
#[inline(always)]
fn nibble_to_hex(n: u8) -> u8 {
    let mask = ((9u8.wrapping_sub(n)) as i8 >> 7) as u8;
    n + b'0' + (mask & (b'a' - b'0' - 10))
}

#[multiversion::multiversion(targets = "simd")]
fn encode_buf(src: &[u8], dst: &mut [u8]) {
    // Process pairs so the autovectoriser can interleave hi/lo nibble lanes.
    for (i, &b) in src.iter().enumerate() {
        dst[2 * i]     = nibble_to_hex(b >> 4);
        dst[2 * i + 1] = nibble_to_hex(b & 0x0f);
    }
}

// ---- base16 decode ---------------------------------------------------------

// Branchless ASCII hex char → nibble (assumes valid input):
//   nibble = (c & 0x0f) + (c >> 6) * 9
//   '0'..'9': low4 = 0..9, c>>6 = 0     → 0..9
//   'a'..'f': low4 = 1..6, c>>6 = 1     → 10..15
//   'A'..'F': low4 = 1..6, c>>6 = 1     → 10..15
#[inline(always)]
fn hex_to_nibble(c: u8) -> u8 {
    (c & 0x0f) + (c >> 6) * 9
}

#[multiversion::multiversion(targets = "simd")]
fn decode_buf(src: &[u8], dst: &mut [u8]) {
    // src.len() must be even; dst.len() must equal src.len() / 2.
    for (i, pair) in src.chunks_exact(2).enumerate() {
        dst[i] = (hex_to_nibble(pair[0]) << 4) | hex_to_nibble(pair[1]);
    }
}

// ---- main / I/O ------------------------------------------------------------

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let (op, key) = parse_args(&args);

    let stdin_h = io::stdin();
    let mut stdin = stdin_h.lock();
    let stdout_h = io::stdout();
    let mut stdout = BufWriter::new(stdout_h.lock());

    let mut in_buf = vec![0u8; CHUNK];
    let mut out_buf = vec![0u8; CHUNK * 2];
    let mut key_pos = 0usize;

    loop {
        let n = read_full(&mut stdin, &mut in_buf);
        if n == 0 {
            break;
        }

        match op {
            Op::Xor => {
                xor_buf(&mut in_buf[..n], &key, &mut key_pos);
                stdout.write_all(&in_buf[..n]).unwrap_or_else(|e| die_write(e));
            }
            Op::Encode => {
                encode_buf(&in_buf[..n], &mut out_buf[..n * 2]);
                stdout.write_all(&out_buf[..n * 2]).unwrap_or_else(|e| die_write(e));
            }
            Op::Decode => {
                if n % 2 != 0 {
                    eprintln!(
                        "\nError: base16decode input length must be even (got {n} bytes)"
                    );
                    process::exit(1);
                }
                decode_buf(&in_buf[..n], &mut out_buf[..n / 2]);
                stdout.write_all(&out_buf[..n / 2]).unwrap_or_else(|e| die_write(e));
            }
        }
    }
}

fn read_full(r: &mut impl Read, buf: &mut [u8]) -> usize {
    let mut total = 0;
    while total < buf.len() {
        match r.read(&mut buf[total..]) {
            Ok(0) => break,
            Ok(n) => total += n,
            Err(e) if e.kind() == io::ErrorKind::Interrupted => continue,
            Err(e) => {
                eprintln!("Read error: {e}");
                process::exit(2);
            }
        }
    }
    total
}

fn die_write(e: std::io::Error) -> ! {
    eprintln!("Write error: {e}");
    process::exit(2);
}

fn parse_args(args: &[String]) -> (Op, Vec<u8>) {
    if args.len() < 2 || args[1].eq_ignore_ascii_case("-h") {
        eprint!("{HELP}");
        process::exit(1);
    }
    let arg = &args[1];
    if arg.eq_ignore_ascii_case("base16encode") {
        return (Op::Encode, vec![]);
    }
    if arg.eq_ignore_ascii_case("base16decode") {
        return (Op::Decode, vec![]);
    }
    let key = arg.as_bytes().to_vec();
    if key.is_empty() {
        eprintln!("\nError: XOR key cannot be empty");
        process::exit(1);
    }
    (Op::Xor, key)
}

const HELP: &str = "\
XOR-encrypt[base16-encode|decode] stdin. Dmitry Unltd. (c)2006 -- Rust/SIMD (multiversion)

USAGE:
      xor_rust_simd 'password' or [base16encode|base16decode]
      -Data read from stdin, written to stdout; diagnostics to stderr
      -Binary files supported; key must be non-empty
      -Best SIMD ISA selected at runtime (SSE2/AVX/AVX2/AVX-512/NEON/...)

EXAMPLES:
  encrypt:  xor_rust_simd password <in >out
  decrypt:  xor_rust_simd password <enc >dec
  pipeline: echo foo | xor_rust_simd pwd | xor_rust_simd base16encode | \\
            xor_rust_simd base16decode | xor_rust_simd pwd
";

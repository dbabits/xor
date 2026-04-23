use std::io::{self, Read, Write, BufWriter};
use std::process;

const CHUNK: usize = 65536;
const HEX: &[u8; 16] = b"0123456789abcdef";

const fn make_unhex() -> [u8; 256] {
    let mut t = [0xffu8; 256];
    let mut i = 0usize;
    while i < 10 { t[b'0' as usize + i] = i as u8; i += 1; }
    i = 0;
    while i < 6 {
        t[b'a' as usize + i] = (10 + i) as u8;
        t[b'A' as usize + i] = (10 + i) as u8;
        i += 1;
    }
    t
}
const UNHEX: [u8; 256] = make_unhex();

enum Op { Xor, Encode, Decode }

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let (op, key) = parse_args(&args);

    let stdin_h = io::stdin();
    let mut stdin = stdin_h.lock();
    let stdout_h = io::stdout();
    let mut stdout = BufWriter::new(stdout_h.lock());

    let mut in_buf  = vec![0u8; CHUNK];
    let mut out_buf = vec![0u8; CHUNK * 2];
    let mut key_pos = 0usize;
    let klen = key.len();

    loop {
        let n = read_full(&mut stdin, &mut in_buf);
        if n == 0 { break; }

        match op {
            Op::Xor => {
                xor_buf(&mut in_buf[..n], &key, &mut key_pos, klen);
                stdout.write_all(&in_buf[..n]).unwrap_or_else(|e| die_write(e));
            }
            Op::Encode => {
                encode_buf(&in_buf[..n], &mut out_buf[..n * 2]);
                stdout.write_all(&out_buf[..n * 2]).unwrap_or_else(|e| die_write(e));
            }
            Op::Decode => {
                if n % 2 != 0 {
                    eprintln!("\nError: base16decode input length must be even (got {n} bytes)");
                    process::exit(1);
                }
                decode_buf(&in_buf[..n], &mut out_buf[..n / 2]);
                stdout.write_all(&out_buf[..n / 2]).unwrap_or_else(|e| die_write(e));
            }
        }
    }
}

// ── XOR ──────────────────────────────────────────────────────────────────────
// NEON tile path when 16 % klen == 0 (klen ∈ {1,2,4,8,16}); scalar otherwise.
// For tile-eligible key lengths, CHUNK (65536) is also divisible by klen, so
// key_pos returns to its starting value after each full chunk.
fn xor_buf(buf: &mut [u8], key: &[u8], key_pos: &mut usize, klen: usize) {
    #[cfg(target_arch = "aarch64")]
    if 16 % klen == 0 {
        unsafe { xor_neon(buf, key, key_pos, klen); }
        return;
    }
    // scalar path
    for b in buf.iter_mut() {
        *b ^= key[*key_pos];
        *key_pos += 1;
        if *key_pos == klen { *key_pos = 0; }
    }
}

#[cfg(target_arch = "aarch64")]
unsafe fn xor_neon(buf: &mut [u8], key: &[u8], key_pos: &mut usize, klen: usize) {
    use std::arch::aarch64::*;

    // Build 16-byte tile from key_pos onward
    let mut tile = [0u8; 16];
    let start = *key_pos;
    for i in 0..16 { tile[i] = key[(start + i) % klen]; }
    let key_vec = vld1q_u8(tile.as_ptr());

    let ptr = buf.as_mut_ptr();
    let n = buf.len();
    let mut i = 0usize;

    while i + 16 <= n {
        let data = vld1q_u8(ptr.add(i));
        vst1q_u8(ptr.add(i), veorq_u8(data, key_vec));
        i += 16;
    }
    // scalar tail — key_pos at start of tail = start (16 % klen == 0, so no drift)
    let mut j = start;
    while i < n {
        *ptr.add(i) ^= key[j];
        j += 1;
        if j == klen { j = 0; }
        i += 1;
    }
    *key_pos = (start + n) % klen;
}

// ── base16encode ─────────────────────────────────────────────────────────────
// NEON: 8 input bytes → 16 hex chars via TBL; scalar tail.
fn encode_buf(input: &[u8], out: &mut [u8]) {
    #[cfg(target_arch = "aarch64")]
    unsafe { encode_neon(input, out); return; }
    #[cfg(not(target_arch = "aarch64"))]
    encode_scalar(input, out);
}

#[cfg(target_arch = "aarch64")]
unsafe fn encode_neon(input: &[u8], out: &mut [u8]) {
    use std::arch::aarch64::*;

    let hex_tbl = vld1q_u8(HEX.as_ptr());   // 16-byte TBL table
    let mask_lo = vdup_n_u8(0x0f);           // nibble mask (8-byte)

    let in_ptr  = input.as_ptr();
    let out_ptr = out.as_mut_ptr();
    let n = input.len();
    let mut i = 0usize;

    // 8 bytes → 16 hex chars per iteration
    while i + 8 <= n {
        let bytes = vld1_u8(in_ptr.add(i));          // uint8x8_t: 8 input bytes
        let hi    = vshr_n_u8::<4>(bytes);            // high nibbles
        let lo    = vand_u8(bytes, mask_lo);          // low nibbles
        // interleave: [hi0,lo0, hi1,lo1, ..., hi7,lo7] — 16 nibble indices
        let zipped  = vzip_u8(hi, lo);                // uint8x8x2_t
        let nibbles = vcombine_u8(zipped.0, zipped.1);// uint8x16_t
        let hex_out = vqtbl1q_u8(hex_tbl, nibbles);   // TBL lookup → 16 hex chars
        vst1q_u8(out_ptr.add(i * 2), hex_out);
        i += 8;
    }
    // scalar tail
    while i < n {
        let b = *in_ptr.add(i);
        *out_ptr.add(i * 2)     = HEX[(b >> 4) as usize];
        *out_ptr.add(i * 2 + 1) = HEX[(b & 0xf) as usize];
        i += 1;
    }
}

#[cfg(not(target_arch = "aarch64"))]
fn encode_scalar(input: &[u8], out: &mut [u8]) {
    for (i, &b) in input.iter().enumerate() {
        out[i * 2]     = HEX[(b >> 4) as usize];
        out[i * 2 + 1] = HEX[(b & 0xf) as usize];
    }
}

// ── base16decode ─────────────────────────────────────────────────────────────
// NEON: 16 hex chars → 8 bytes via branchless nibble formula; scalar tail.
// nibble = (char & 0x0f) + (char >> 6) * 9
//   '0'=0x30: (0)+(0*9)= 0  …  '9'=0x39: (9)+(0*9)= 9
//   'a'=0x61: (1)+(1*9)=10  …  'f'=0x66: (6)+(1*9)=15
//   'A'=0x41: (1)+(1*9)=10  …  'F'=0x46: (6)+(1*9)=15
fn decode_buf(input: &[u8], out: &mut [u8]) {
    #[cfg(target_arch = "aarch64")]
    unsafe { decode_neon(input, out); return; }
    #[cfg(not(target_arch = "aarch64"))]
    decode_scalar(input, out);
}

#[cfg(target_arch = "aarch64")]
unsafe fn decode_neon(input: &[u8], out: &mut [u8]) {
    use std::arch::aarch64::*;

    let mask_0f = vdupq_n_u8(0x0f);
    let nine    = vdupq_n_u8(9);

    let in_ptr  = input.as_ptr();
    let out_ptr = out.as_mut_ptr();
    let n_out   = out.len();   // = input.len() / 2
    let mut i   = 0usize;     // output byte index

    // 16 hex chars → 8 decoded bytes per iteration
    while i + 8 <= n_out {
        let chars   = vld1q_u8(in_ptr.add(i * 2));        // 16 hex chars
        let lo      = vandq_u8(chars, mask_0f);            // char & 0x0f
        let bit6    = vshrq_n_u8::<6>(chars);              // char >> 6  (0 or 1)
        let adj     = vmulq_u8(bit6, nine);                // 0 or 9
        let nibbles = vaddq_u8(lo, adj);                   // 16 nibbles
        // de-interleave: even positions → hi nibbles, odd positions → lo nibbles
        let uzp     = vuzpq_u8(nibbles, nibbles);          // uint8x16x2_t
        let hi_n    = vget_low_u8(uzp.0);                  // 8 hi nibbles
        let lo_n    = vget_low_u8(uzp.1);                  // 8 lo nibbles
        let result  = vorr_u8(vshl_n_u8::<4>(hi_n), lo_n);// (hi<<4)|lo
        vst1_u8(out_ptr.add(i), result);
        i += 8;
    }
    // scalar tail
    while i < n_out {
        let hi = UNHEX[*in_ptr.add(i * 2) as usize];
        let lo = UNHEX[*in_ptr.add(i * 2 + 1) as usize];
        *out_ptr.add(i) = (hi << 4) | lo;
        i += 1;
    }
}

#[cfg(not(target_arch = "aarch64"))]
fn decode_scalar(input: &[u8], out: &mut [u8]) {
    for i in 0..out.len() {
        let hi = UNHEX[input[i * 2] as usize];
        let lo = UNHEX[input[i * 2 + 1] as usize];
        out[i] = (hi << 4) | lo;
    }
}

// ── utilities ────────────────────────────────────────────────────────────────
fn read_full(r: &mut impl Read, buf: &mut [u8]) -> usize {
    let mut total = 0;
    while total < buf.len() {
        match r.read(&mut buf[total..]) {
            Ok(0) => break,
            Ok(n) => total += n,
            Err(e) if e.kind() == io::ErrorKind::Interrupted => continue,
            Err(e) => { eprintln!("Read error: {e}"); process::exit(2); }
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
    if arg.eq_ignore_ascii_case("base16encode") { return (Op::Encode, vec![]); }
    if arg.eq_ignore_ascii_case("base16decode") { return (Op::Decode, vec![]); }
    let key = arg.as_bytes().to_vec();
    if key.is_empty() {
        eprintln!("\nError: XOR key cannot be empty");
        process::exit(1);
    }
    (Op::Xor, key)
}

const HELP: &str = "\
XOR-encrypt[base16-encode|decode] stdin. Dmitry Unltd. (c)2006 -- Rust/NEON

USAGE:
      xor_rust_neon 'password' or [base16encode|base16decode]
      -Data read from stdin, written to stdout; diagnostics to stderr
      -Binary files supported; key must be non-empty
      -ARM64 NEON SIMD used for XOR (tile), encode (TBL), decode (arithmetic)

EXAMPLES:
  encrypt:  xor_rust_neon password <in >out
  decrypt:  xor_rust_neon password <enc >dec
  pipeline: echo foo | xor_rust_neon pwd | xor_rust_neon base16encode | \\
            xor_rust_neon base16decode | xor_rust_neon pwd
";

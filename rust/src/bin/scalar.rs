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
                for b in in_buf[..n].iter_mut() {
                    *b ^= key[key_pos];
                    key_pos += 1;
                    if key_pos == klen { key_pos = 0; }
                }
                stdout.write_all(&in_buf[..n]).unwrap_or_else(|e| die_write(e));
            }
            Op::Encode => {
                for (i, &b) in in_buf[..n].iter().enumerate() {
                    out_buf[i * 2]     = HEX[(b >> 4) as usize];
                    out_buf[i * 2 + 1] = HEX[(b & 0xf) as usize];
                }
                stdout.write_all(&out_buf[..n * 2]).unwrap_or_else(|e| die_write(e));
            }
            Op::Decode => {
                if n % 2 != 0 {
                    eprintln!("\nError: base16decode input length must be even (got {n} bytes)");
                    process::exit(1);
                }
                for i in 0..n / 2 {
                    let hi = UNHEX[in_buf[i * 2] as usize];
                    let lo = UNHEX[in_buf[i * 2 + 1] as usize];
                    out_buf[i] = (hi << 4) | lo;
                }
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
XOR-encrypt[base16-encode|decode] stdin. Dmitry Unltd. (c)2006 -- Rust/scalar

USAGE:
      xor_rust_scalar 'password' or [base16encode|base16decode]
      -Data read from stdin, written to stdout; diagnostics to stderr
      -Binary files supported; key must be non-empty

EXAMPLES:
  encrypt:  xor_rust_scalar password <in >out
  decrypt:  xor_rust_scalar password <enc >dec
  pipeline: echo foo | xor_rust_scalar pwd | xor_rust_scalar base16encode | \\
            xor_rust_scalar base16decode | xor_rust_scalar pwd
";

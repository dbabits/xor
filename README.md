# xor
Automatically exported from code.google.com/p/xore

```
$ xor

XOR-encrypt[base16-encode|decode] stdin. Dmitry Unltd. ©2006

USAGE:
      xor 'password' or [base16encode|base16decode]
      -if password is one of these: base16encode|base16decode the program encodes/decodes instead of xor
      -Data to be encrypted/decrypted/encoded/decoded is read from stdin and written to stdout
      -Diagnostic messages are written to stderr, redirect 2>/dev/null (Unix) if you don't want them
      -Binary files no problem. Key also could be binary, but then can't pass it as an arg

EXAMPLES:
  encrypt:
      xor password <test.original > test.encrypted
  decrypt:
      xor password <test.encrypted> test.decrypted
  check(should be no diff):
      diff test.original test.decrypted
  encrypt|base16encode|base16decode|decrypt->get original text:
      echo foo|xor pwd|xor base16encode|xor base16decode|xor pwd

INFO:
      This tool is intended as a demonstration and a playground for exploring different
      implementation techniques (C, ARM64 assembly, NEON SIMD, Rust) — not as a replacement
      for production-grade encryption. For real security needs, use a standard cipher such
      as AES-GCM or ChaCha20-Poly1305.

      XOR encryption works by combining each byte of the input with a byte from a repeating
      key. For example, with plaintext STARS and key ABC:
      (S XOR A)(T XOR B)(A XOR C)(R XOR A)(S XOR B).
      Because XOR is its own inverse, the same operation decrypts: applying the key again
      recovers the original plaintext.

      More information: https://en.wikipedia.org/wiki/XOR_cipher
```

## Performance optimizations — C implementation (2026)

Step-by-step history of the C binary (`xor.cpp`), benchmarked on Raspberry Pi 5,
100 MB input, median of 3 runs per commit.

![Baseline vs final throughput, and cumulative progression (C)](bench_combined.png)

<details><summary>Step-by-step progression with binary size</summary>

![Throughput and binary size across commits](bench_results.png)

</details>

| Commit | XOR MB/s | Encode MB/s | Decode MB/s | Binary (KB) |
|---|---|---|---|---|
| baseline | 438 | 14 | 78 | 71.8 |
| 1. buffer 64KB | 515 | 14 | 79 | 71.8 |
| 2. encode LUT | 493 | 395 | 78 | 71.9 |
| 3. decode LUT | 505 | 392 | 897 | 71.9 |
| 4. dead code removed | 490 | 426 | 948 | 69.7 |
| 5. -Os | 518 | 388 | 803 | 69.7 |
| 6. strip | 513 | 392 | 766 | 66.1 |

### What moved the needle

**Encode LUT (+2762%):** The biggest win by far. `snprintf("%2.2x", b)` is catastrophically expensive
per byte — it parses a format string, handles locale, performs bounds checking, and writes through a
`FILE*`-like abstraction. Two array lookups and two stores cost essentially nothing. The baseline of
14 MB/s was pure format-string overhead.

**Decode LUT (+1050%):** `strtol` is similarly over-engineered for the job — it handles signs, prefixes
(`0x`), overflow detection, and locale. A 256-entry nibble lookup table reduces each pair of hex chars
to two array reads, a shift, and an OR. No branching, no allocation.

**Buffer 64KB (+18%, XOR only):** Helped XOR noticeably because XOR's inner loop is cheap — syscall
overhead was a meaningful fraction of total time. Had no measurable effect on encode/decode because those
were CPU-bound on `snprintf`/`strtol`, not I/O-bound.

**Dead code removal:** No runtime effect (the removed functions were never called). Binary shrank ~2KB
from dropping `<string>` and `<sstream>`.

**-Os and strip:** Pure size plays. Strip took the binary from 71.8 KB to 66.1 KB by removing the symbol
table and debug section headers. `-Os` vs `-O2` made no measurable throughput difference — the hot paths
(LUT indexing, fread/fwrite) are already as simple as the compiler can make them.

## All implementations compared (2026)

ARM64 assembly and Rust added alongside the C baseline. All measured fresh on Raspberry Pi 5,
100 MB input, median of 3 runs. Scalar ASM numbers are from an earlier session (that build was
superseded by the NEON version).

![All implementations — throughput comparison](bench_asm_comparison.png)

| Implementation  | XOR MB/s | Encode MB/s | Decode MB/s | Binary (KB) |
|---|---|---|---|---|
| Scalar ASM *(historical)* |  521 |  394 |  719 |  66 |
| C (gcc -Os)               |  503 |  452 |  797 |  66 |
| NEON ASM                  | 2778 | 1351 | 2128 |  66 |
| Rust (scalar)             |  467 |  485 | 1156 | 325 |
| **Rust (NEON)**           | **4167** | **1282** | **1653** | 325 |
| Rust NEON vs C            | **+8.3x** | **+2.8x** | **+2.1x** | |
| Rust NEON vs NEON ASM     | **+50%** | -5% | -22% | |

### What each technique does

**Scalar ASM loses to the compiler** on encode and decode. gcc's instruction scheduler produces
better code for simple LUT loops. Manual assembly only pays off when the compiler can't vectorize.

**NEON — three different strategies, one per function:**

- **XOR — key tile + EOR v.16b:** Build a 16-byte repeating key tile once, then XOR 16 data bytes
  per instruction. Scalar costs ~5 instructions/byte; NEON amortises that to ~5 per 16 bytes.
  The 64 KB working set fits in L1 cache → essentially L1 bandwidth.

- **Encode — TBL lookup:** `TBL v.16b, {hex_table}, indices` performs 16 simultaneous nibble→hex
  lookups in one instruction. Nibble indices prepared with `USHR`+`AND`+`ZIP1`; entire 8-byte →
  16-char conversion in one `TBL` + one `STR Q`.

- **Decode — branchless arithmetic:** The unhex table is 256 bytes — too large for TBL. Instead:
  `nibble = (char & 0x0f) + (char >> 6) * 9` works for `'0'-'9'`, `'a'-'f'`, `'A'-'F'`.
  `UZP1`/`UZP2` de-interleave hi/lo nibbles; `SHL`+`ORR` combine into output bytes.
  16 chars → 8 bytes per NEON pass.

**Rust scalar beats C on encode (+7%) and decode (+45%).** LLVM at `-O3` with LTO auto-vectorizes
the nibble loops that gcc `-Os` leaves scalar — no hand-written SIMD needed.

**Rust NEON XOR outpaces hand-written assembly (+50%).** Same `EOR` tile strategy; LLVM unrolls
the loop more aggressively than the hand-written version, hiding more memory latency.

**Rust NEON encode and decode are within ~5–22% of ASM NEON** — using the same intrinsics
(`VQTBL1`, `UZP`, `SHL`, `ORR`) but with LLVM register allocation vs hand-scheduled ASM.

**Rust binary size: 325 KB vs 66 KB.** Static linking brings in the standard library runtime.
The ~260 KB overhead is fixed regardless of application code size.

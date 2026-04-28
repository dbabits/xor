# xor
Automatically exported from code.google.com/p/xore

```
$ xor

XOR-encrypt[base16-encode|decode] stdin. Dmitry Unltd. ©2006

USAGE:
      xor 'password' or [base16encode|base16decode]
      -if password is one of these: base16encode|base16decode the program encodes/decodes instead of xor
      -Data to be encrypted/decrypted/encoded/decoded is read from stdin and written to stdout
      -Diagnostic messages are written to stderr, redirect 2>/dev/null (Unix) or 2>NUL (Windows) if you don't want it
      -Binary files no problem. Key also could be binary, but then can't pass it as an arg

EXAMPLES:
  encrypt:
      xor password <test.original > test.encrypted
  decrypt:
      xor password <test.encrypted> test.decrypted
  check(should be no diff):
      diff test.original test.decrypted
  interactive use-type or paste your text,terminate by 'Enter' and ^D (Unix) or ^Z (Windows):
      xor password > test.encrypted
  encrypt|base16encode|base16decode|decrypt->get original text:
      echo foo|xor pwd|xor base16encode|xor base16decode|xor pwd
      xor foobarfoobar < xor.cpp |xor base16encode|xor base16decode|xor foobarfoobar >xor.cpp.fullcircle && diff -s xor.cpp xor.cpp.fullcircle

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

## Portable SIMD via runtime dispatch — `xor_rust_simd`

The `xor_rust_neon` binary is hard-gated on `#[cfg(target_arch = "aarch64")]`: on Apple
Silicon and Pi 5 it uses NEON, but on Intel/AMD x86_64 it falls all the way back to scalar
even though the CPU has SSE/AVX. To get one source file that compiles for every target
*and* picks the best SIMD ISA at runtime, `rust/src/bin/simd.rs` uses the
[`multiversion`](https://crates.io/crates/multiversion) macro:

```rust
#[multiversion::multiversion(targets = "simd")]
fn xor_tiled_16(buf: &mut [u8], tile: &[u8; 16]) {
    for chunk in buf.chunks_exact_mut(16) {
        for j in 0..16 { chunk[j] ^= tile[j]; }
    }
}
```

`targets = "simd"` expands to a clone of the function for each common SIMD level —
`x86_64+sse2`, `+avx`, `+avx2`, `+avx512f`, `aarch64+neon`, `wasm32+simd128`, and a
generic fallback. An atomic-cached function pointer selects the best one on first call.

The bodies are written so LLVM's autovectoriser handles them without
intrinsics:

- **XOR** runs over fixed-stride 16-byte chunks XOR'd against a 16-byte key tile (built
  when `klen` divides 16). LLVM widens the inner unrolled loop to PXOR/VPXOR/VPXOR-zmm/EOR.
- **Encode** uses the branchless nibble-to-hex formula
  `n + b'0' + (mask & 39)` with `mask = (9-n) >> 7` (signed) — pure u8 arithmetic, no
  table lookup, fully vectorisable.
- **Decode** uses the same branchless `(c & 0x0f) + (c >> 6) * 9` formula as the
  hand-written NEON version.

### x86_64 throughput (50 MB input, AVX2 box)

Quick local measurement (medians of 3 runs, on the development VM — not the Pi 5):

| Implementation        | XOR MB/s | Encode MB/s | Decode MB/s |
|---|---|---|---|
| Rust (scalar)         |  714 |  735 | 1666 |
| **Rust (SIMD)**       | **5000** | **2083** | **4761** |
| SIMD vs scalar        | **+7.0×** | **+2.8×** | **+2.9×** |

Same source, no `#[cfg]`, no intrinsics — `multiversion` + LLVM autovectorisation does
all the work. The same source built for aarch64 picks the NEON clone instead of the
AVX2 one.

**Actual Pi 5 numbers** (Cortex-A76, 100 MB input, median of 3 runs):

| | XOR MB/s | Encode MB/s | Decode MB/s |
|---|---|---|---|
| Rust (NEON) — hand-written intrinsics | 2439 | 826 | 1550 |
| Rust (SIMD/multiversion) | 1429 | 820 | 1754 |
| multiversion vs NEON | **-41%** | ~0% | **+13%** |

Encode and decode land close to the hand-written NEON build; XOR is 41% slower because
the autovectoriser does not unroll the tile loop as aggressively as explicit `veorq_u8`
intrinsics. If XOR throughput is the priority, `xor_rust_neon` remains the ceiling.

### What "portable" means here

`multiversion` does runtime dispatch **within an architecture** — picking AVX2 vs SSE4.1
vs SSE2 once you're already on x86_64, or NEON vs scalar once you're already on aarch64.
It does **not** make one ELF/PE/Mach-O run on both Intel and ARM CPUs — those are
different machine codes.

What you actually get from one source file:

```sh
cargo build --release --target=x86_64-unknown-linux-gnu     # Linux Intel/AMD
cargo build --release --target=aarch64-unknown-linux-gnu    # Linux ARM (Pi 5, Graviton)
cargo build --release --target=x86_64-apple-darwin          # Intel Mac
cargo build --release --target=aarch64-apple-darwin         # Apple Silicon Mac
cargo build --release --target=x86_64-pc-windows-msvc       # Windows Intel/AMD
```

→ N binaries, one per (arch, OS), each of which runtime-dispatches the best SIMD ISA
the host CPU supports. The only mainstream "one file, multiple CPU archs" deployment
is **macOS universal binaries**, which you build by combining the two `apple-darwin`
outputs with `lipo`:

```sh
lipo -create \
  target/x86_64-apple-darwin/release/xor_rust_simd \
  target/aarch64-apple-darwin/release/xor_rust_simd \
  -output xor_rust_simd_universal
```

Linux ELF and Windows PE don't have an analogue — you ship two artifacts and let the
package manager / container manifest / installer pick the right one.

### When this is the right choice

- **One source, many target binaries**, each maximally vectorised for its host CPU,
  with no per-arch `#[cfg]` blocks to maintain.
- **Adding AVX-512 or SVE later is just another entry in the targets list** — no new code.
- **You can tolerate ~10–20% off the hand-written ceiling** in exchange for portability.

### When to drop down to intrinsics instead

- **Encode is hot enough that you need the TBL/PSHUFB single-instruction lookup.**
  Autovectorised arithmetic gets close but not all the way to the NEON `TBL` performance.
- **You need a SIMD primitive LLVM can't pattern-match** (gather, masked store on stable,
  fancy permute). Then write a `WithSimd` impl in `pulp` or fall back to `std::arch::*`
  inside `#[target_feature]` functions.

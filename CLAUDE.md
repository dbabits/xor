# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Command-line utility (originally C++, 2006) that performs XOR encryption and base16 (hex) encoding/decoding on stdin, writing results to stdout. Now includes three implementations: C, ARM64 NEON assembly, and Rust (scalar + NEON intrinsics), used as a playground for comparing optimization techniques.

## Building

**Linux/macOS** — use the `Makefile`:
```sh
make        # builds ./xor (C)
make asm    # builds ./xor_asm (ARM64 NEON assembly, requires aarch64)
make rust   # builds rust/target/release/xor_rust_scalar and xor_rust_neon
make test   # builds C binary and runs test.sh
make clean  # remove build artifacts
```

**Windows** — use the included Visual Studio project files (`xor.vcproj`, `xor.sln`, `xor.dsp`, `xor.dsw`) with MSVC. Only the C implementation.

`StdAfx.h` handles portability: on non-Windows it defines `_setmode`/`_fileno`/`_O_BINARY` as no-ops (Unix stdin/stdout are already binary) and maps `strcasecmp` to `_stricmp`. `BYTE` is typedef'd to `uint8_t`.

## Code Structure

### C implementation (`xor.cpp`, `StdAfx.h`, `StdAfx.cpp`)

- `xor_encrypt(buffer, bufsize, key, keysize)` — in-place XOR of a byte buffer against a cycling key
- `base16encode(buffer_in, bufinsize, buffer_out, bufoutsize)` — converts bytes to lowercase hex via a 16-byte LUT
- `base16decode(base16buf, bufinsize, buffer_out, bufoutsize)` — converts hex back to bytes via a 256-entry nibble LUT
- `main()` — reads 65536-byte chunks from stdin in a loop, dispatches by `argv[1]`, writes to stdout

Dispatch logic: if `argv[1]` is `"base16encode"` or `"base16decode"` (case-insensitive), it encodes/decodes; otherwise it treats `argv[1]` as the XOR key.

### ARM64 NEON assembly (`asm/xor_asm.s`, `asm/main.cpp`)

Same three functions implemented in ARMv8-A assembly with NEON SIMD instructions. `main.cpp` is a C wrapper that calls the assembly routines via `extern "C"`. Key techniques: `EOR v.16b` for XOR with key tiling, `TBL` for encode lookups, branchless arithmetic for decode.

### Rust (`rust/src/bin/scalar.rs`, `rust/src/bin/neon.rs`)

Two binaries: `xor_rust_scalar` (pure Rust) and `xor_rust_neon` (conditional `#[cfg(target_arch = "aarch64")]` NEON intrinsics with scalar fallback). Built with `opt-level = 3`, LTO, and strip.

## Testing

`test.sh` — 25 integration tests covering exit codes, known-vector encode/decode, round-trips (XOR, encode/decode, full pipeline), multi-chunk key cycling across 65536-byte boundaries, odd-input error handling, and case-insensitive keyword matching. Run with `make test` or `bash test.sh [path-to-binary]`.

## Benchmarking

`bench.sh` — measures XOR (100 MB), encode (100 MB), and decode (200 MB hex) throughput; 3 trials, reports median MB/s. `pi5_run.sh` automates the full build-test-bench-commit cycle on Raspberry Pi 5. Results in `results.txt` and `results_x86.txt`; plotted by `plot_*.py` scripts (matplotlib).

## Usage

```sh
xor password <input >output        # encrypt
xor password <encrypted >decrypted # decrypt (same operation)
xor base16encode <input >output    # hex-encode
xor base16decode <input >output    # hex-decode
```

Diagnostic output goes to stderr; redirect with `2>/dev/null` (Unix) or `2>NUL` (Windows) to suppress.

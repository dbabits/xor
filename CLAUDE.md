# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This is a small C++ command-line utility (originally from 2006) that performs XOR encryption and base16 (hex) encoding/decoding on stdin, writing results to stdout.

## Building

**Linux/macOS** — use the `Makefile`:
```sh
make        # builds ./xor
make clean  # remove build artifacts
```

**Windows** — use the included Visual Studio project files (`xor.vcproj`, `xor.sln`, `xor.dsp`, `xor.dsw`) with MSVC.

`StdAfx.h` handles portability: on non-Windows it defines `_setmode`/`_fileno`/`_O_BINARY` as no-ops (Unix stdin/stdout are already binary) and maps `_sntprintf`/`_T()` to their standard equivalents. `BYTE` is typedef'd to `uint8_t`.

## Code Structure

All logic lives in a single file: `xor.cpp` (with `StdAfx.h`/`StdAfx.cpp` for the precompiled header).

Key functions in `xor.cpp`:
- `xor_encrypt(buffer, bufsize, key, keysize)` — in-place XOR of a byte buffer against a cycling key
- `base16encode(buffer_in, bufinsize, buffer_out, bufoutsize)` — converts bytes to lowercase hex string; `bufoutsize` must be `bufinsize*2+1` (extra byte for null terminator)
- `base16decode(base16buf, bufinsize, buffer_out, bufoutsize)` — converts hex string back to bytes using `strtol`
- `main()` — reads 4096-byte chunks from stdin in a loop, applies the selected operation, writes to stdout

The `main` dispatch logic: if `argv[1]` is `"base16encode"` or `"base16decode"`, it encodes/decodes; otherwise it treats `argv[1]` as the XOR key.

## Usage

```sh
xor password <input >output        # encrypt
xor password <encrypted >decrypted # decrypt (same operation)
xor base16encode <input >output    # hex-encode
xor base16decode <input >output    # hex-decode

# full round-trip example:
echo foo | xor pwd | xor base16encode | xor base16decode | xor pwd
```

Diagnostic output goes to stderr; redirect with `2>/dev/null` (Unix) or `2>NUL` (Windows) to suppress it.

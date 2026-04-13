# xor
Automatically exported from code.google.com/p/xore

```
This is a Windows program demonstrating xor encryption.
It should be easy to port to other OSs, as there's not much if anything Windows-specific.
(I however did not try to cross-compile it)
Binary file is in Releases
c:\>xor

XOR-encrypt[base16-encode|decode] stdin. Dmitry Unltd. ⌐2006

USAGE:
      xor 'password' or [base16encode|base16decode]
      -if password is one of these: base16encode|base16decode the program encodes/decodes instead of xor
      -Data to be encrypted/decrypted/encoded/decoded is read from stdin and written to stdout
      -Diagnostic messages are written to stderr, redirect 2>/dev/null (Unix) or 2>NUL (Windows) if you don't want them
      -Binary files no problem. Key also could be binary, but then can't pass it as an arg

EXAMPLES:
  encrypt:
      xor password <test.original > test.encrypted
  decrypt:
      xor password <test.encrypted> test.decrypted
  check(should be no diff):
      diff test.original test.decrypted
  interactive use-type or paste your text,terminate by 'Enter' and ^Z:
      xor password > test.encrypted
  encrypt|base16encode|base16decode|decrypt->get original text:
      echo foo|xor pwd|xor base16encode|xor base16decode|xor pwd
      xor foobarfoobar < xor.cpp |xor base16encode|xor base16decode|xor foobarfoobar >xor.cpp.fullcircle && diff -s xor.cpp xor.cpp.fullcircle

INFO:
      XOR-encryption is very simple and quite strong. Search Google for more on XOR encryption.
      The encryption algorithm runs through each letter of the unencrypted phrase and XOR's it
      with one letter of the key. For example, if the unencrypted phrase was
      STARS, and the key was ABC, the encryption algorithm would go something like
      this: (S XOR A)(T XOR B)(A XOR C)(R XOR A)(S XOR B). XOR only works with two
      single letters at a time, which is why the algorithm needs to split both the
      phrase and the key letter by letter. Because of the nature of the algorithm,
      the length of the encrypted phrase is the same length as the unencrypted
      phrase.The beauty of XOR encryption comes in its decryption. The algorithm
      for encryption is the SAME as the one for decryption. For decryption, the
      key is XOR'ed against the encrypted phrase, and the result is the decrypted
      phrase.
```

## Performance optimizations (2026)

Benchmarked on Raspberry Pi 5, 100 MB input, median of 3 runs per commit.

![Baseline vs final throughput, and cumulative progression](bench_combined.png)

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

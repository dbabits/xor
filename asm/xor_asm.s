// ARM64 (AArch64) assembly — xor_encrypt, base16encode, base16decode
// Raspberry Pi 5 (Cortex-A76, ARMv8-A)
// NEON (Advanced SIMD) used throughout.
//
// Calling convention (AAPCS64):
//   args:    x0-x7   (int params arrive in w0-w7)
//   scratch: x9-x15  (caller-saved, free to clobber)
//   saved:   x19-x28 (callee-saved; not touched here)
//   return:  x0
//   NEON:    v0-v7 caller-saved, v8-v15 callee-saved; we only use v0-v6

    .arch armv8-a
    .text

// ─────────────────────────────────────────────────────────────────────────────
// void xor_encrypt(BYTE *buffer, int bufsize, const BYTE *key, int keysize)
//   x0=buffer  w1=bufsize  x2=key  w3=keysize
//
// NEON path (when 16 % keysize == 0: keysize 1,2,4,8,16):
//   Build a 16-byte key tile; XOR 16 data bytes per cycle.
//   After any multiple of 16 input bytes the key position returns to 0
//   (because 16 % keysize == 0), so the same tile is valid forever.
//
// Scalar path (all other key lengths): classic byte-by-byte loop.
// ─────────────────────────────────────────────────────────────────────────────
    .global xor_encrypt
    .type   xor_encrypt, %function
xor_encrypt:
    cbz     w1, .Lxe_ret
    cbz     w3, .Lxe_ret

    // Test 16 % keysize == 0  via  (16/keysize)*keysize == 16
    mov     w9, #16
    udiv    w10, w9, w3
    mul     w10, w10, w3
    cmp     w10, w9
    bne     .Lxe_scalar

    // ── NEON path ────────────────────────────────────────────────────────────
    sub     sp, sp, #16             // 16-byte key tile on stack
    mov     x9, sp                  // tile write pointer
    mov     w10, #0                 // key index
    mov     w11, #16                // tile bytes remaining
.Lxe_tile:
    ldrb    w12, [x2, w10, uxtw]
    strb    w12, [x9], #1
    add     w10, w10, #1
    cmp     w10, w3
    csel    w10, wzr, w10, eq
    subs    w11, w11, #1
    bne     .Lxe_tile

    ldr     q0, [sp]                // key tile → v0
    add     sp, sp, #16

    mov     x9, x0
    mov     w10, w1
.Lxe_neon16:
    cmp     w10, #16
    blt     .Lxe_neon_tail
    ldr     q1, [x9]
    eor     v1.16b, v1.16b, v0.16b
    str     q1, [x9], #16
    sub     w10, w10, #16
    b       .Lxe_neon16
.Lxe_neon_tail:
    cbz     w10, .Lxe_ret
    mov     w11, #0                 // key pos resets to 0 after full 16-byte chunks
.Lxe_neon_rem:
    ldrb    w12, [x9]
    ldrb    w13, [x2, w11, uxtw]
    eor     w12, w12, w13
    strb    w12, [x9], #1
    add     w11, w11, #1
    cmp     w11, w3
    csel    w11, wzr, w11, eq
    subs    w10, w10, #1
    bne     .Lxe_neon_rem
    ret

    // ── Scalar path ──────────────────────────────────────────────────────────
.Lxe_scalar:
    mov     w9,  #0
    mov     w10, #0
.Lxe_sloop:
    ldrb    w11, [x0, w9,  uxtw]
    ldrb    w12, [x2, w10, uxtw]
    eor     w11, w11, w12
    strb    w11, [x0, w9,  uxtw]
    add     w9,  w9,  #1
    add     w10, w10, #1
    cmp     w10, w3
    csel    w10, wzr, w10, eq
    cmp     w9, w1
    blt     .Lxe_sloop
.Lxe_ret:
    ret
    .size   xor_encrypt, . - xor_encrypt


// ─────────────────────────────────────────────────────────────────────────────
// void base16encode(const BYTE *buf_in, int bufinsize,
//                   char *buf_out,      int bufoutsize)
//   x0=buf_in  w1=bufinsize  x2=buf_out  w3=bufoutsize
//
// NEON path — 8 input bytes → 16 hex chars per cycle:
//
//   ldr d1             load 8 bytes (zeros upper half of v1)
//   ushr v2.8b, #4     hi nibbles
//   and  v3.8b, mask   lo nibbles
//   zip1 v4.16b        interleave: [hi0,lo0, hi1,lo1, ..., hi7,lo7]
//   tbl  v6.16b        lookup all 16 nibbles in hex table simultaneously
//   str  q6            write 16 hex chars
// ─────────────────────────────────────────────────────────────────────────────
    .global base16encode
    .type   base16encode, %function
base16encode:
    cbz     w1, .Lbe_ret

    // load hex table and nibble mask into NEON registers once
    adr     x9, .Lhex
    ldr     q0, [x9]            // v0 = "0123456789abcdef" (16-byte table)
    movi    v5.16b, #0x0f       // nibble mask

    mov     x9, x0              // src ptr
    mov     x10, x2             // dst ptr
    mov     w11, w1             // remaining input bytes

.Lbe_neon8:
    cmp     w11, #8
    blt     .Lbe_tail
    ldr     d1, [x9], #8       // 8 bytes into lower half of v1; upper half zeroed
    ushr    v2.8b, v1.8b, #4   // hi nibbles (upper 4 bits of each byte)
    and     v3.8b, v1.8b, v5.8b // lo nibbles (lower 4 bits)
    // zip1 interleaves lower 8 bytes of each register:
    //   v4 = [hi0,lo0, hi1,lo1, ..., hi7,lo7]  — 16 nibbles ready for TBL
    zip1    v4.16b, v2.16b, v3.16b
    // TBL: for each of the 16 nibbles, look up v0[nibble] = hex character
    tbl     v6.16b, {v0.16b}, v4.16b
    str     q6, [x10], #16     // 16 hex chars out
    sub     w11, w11, #8
    b       .Lbe_neon8

    // scalar tail for remaining 1-7 bytes
.Lbe_tail:
    cbz     w11, .Lbe_null
    adr     x12, .Lhex
.Lbe_scalar:
    ldrb    w6, [x9], #1
    lsr     w7, w6, #4
    and     w8, w6, #0xf
    ldrb    w7, [x12, w7, uxtw]
    ldrb    w8, [x12, w8, uxtw]
    strb    w7, [x10], #1
    strb    w8, [x10], #1
    subs    w11, w11, #1
    bne     .Lbe_scalar
.Lbe_null:
    lsl     w5, w1, #1
    strb    wzr, [x2, w5, uxtw] // null-terminate: buf_out[bufinsize*2] = '\0'
.Lbe_ret:
    ret
    .size   base16encode, . - base16encode


// ─────────────────────────────────────────────────────────────────────────────
// BYTE* base16decode(const char *src, int bufinsize,
//                   BYTE *dst,        int bufoutsize)
//   x0=src  w1=bufinsize  x2=dst  w3=bufoutsize
//   returns x0 = original dst
//
// NEON path — 16 hex input chars → 8 bytes per cycle.
//
// Nibble conversion (branchless, works for 0-9, a-f, A-F):
//   nibble = (char & 0x0f) + (char >> 6) * 9
//   '0'=0x30: (0x0) + 0*9 =  0 … '9'=0x39: (0x9) + 0*9 = 9
//   'a'=0x61: (0x1) + 1*9 = 10 … 'f'=0x66: (0x6) + 1*9 = 15
//   'A'=0x41: (0x1) + 1*9 = 10 … 'F'=0x46: (0x6) + 1*9 = 15
//
// After converting 16 chars to 16 nibbles:
//   uzp1 → lower 8 bytes = even-position nibbles = hi nibbles of output bytes
//   uzp2 → lower 8 bytes = odd-position nibbles  = lo nibbles of output bytes
//   shl hi << 4 ; orr hi, lo → 8 output bytes
// ─────────────────────────────────────────────────────────────────────────────
    .global base16decode
    .type   base16decode, %function
base16decode:
    mov     x4, x2              // save original dst for return value
    cbz     w1, .Lbd_ret

    movi    v5.16b, #0x0f       // mask  for (char & 0x0f)
    movi    v6.16b, #9          // const for (char >> 6) * 9

    lsr     w11, w1, #1         // w11 = output byte count = bufinsize/2

.Lbd_neon8:
    cmp     w11, #8
    blt     .Lbd_tail
    ldr     q1, [x0], #16      // 16 hex input chars
    and     v2.16b, v1.16b, v5.16b // char & 0x0f
    ushr    v3.16b, v1.16b, #6  // char >> 6  (0 for 0-9/A-F lower, 1 for a-f)
    mul     v3.16b, v3.16b, v6.16b // (char >> 6) * 9
    add     v2.16b, v2.16b, v3.16b // 16 nibbles
    // de-interleave: even positions → hi nibbles, odd positions → lo nibbles
    uzp1    v3.16b, v2.16b, v2.16b // v3[0..7] = nibbles at positions 0,2,4,...,14
    uzp2    v4.16b, v2.16b, v2.16b // v4[0..7] = nibbles at positions 1,3,5,...,15
    shl     v3.8b, v3.8b, #4    // hi nibbles << 4
    orr     v3.8b, v3.8b, v4.8b // (hi<<4) | lo = 8 decoded bytes
    str     d3, [x2], #8        // write 8 output bytes
    sub     w11, w11, #8
    b       .Lbd_neon8

    // scalar tail for remaining 1-7 output bytes
.Lbd_tail:
    cbz     w11, .Lbd_ret
    adr     x5, .Lunhex
.Lbd_scalar:
    ldrb    w6, [x0], #1
    ldrb    w7, [x0], #1
    ldrb    w6, [x5, w6, uxtw]  // hi = unhex[hi_char]
    ldrb    w7, [x5, w7, uxtw]  // lo = unhex[lo_char]
    lsl     w6, w6, #4
    orr     w6, w6, w7
    strb    w6, [x2], #1
    subs    w11, w11, #1
    bne     .Lbd_scalar
.Lbd_ret:
    mov     x0, x4              // return original dst
    ret
    .size   base16decode, . - base16decode


// ─────────────────────────────────────────────────────────────────────────────
// Read-only data
// ─────────────────────────────────────────────────────────────────────────────
    .section .rodata
    .align  4

.Lhex:
    .ascii  "0123456789abcdef"

    // unhex[c]: nibble value for ASCII hex digit c; 0xff = invalid
    // (used only by scalar fallback paths)
.Lunhex:
    .byte 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0x00
    .byte 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0x10
    .byte 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0x20
    .byte 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xff,0xff,0xff,0xff,0xff,0xff  // 0x30  '0'-'9'
    .byte 0xff,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0x40  'A'-'F'
    .byte 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0x50
    .byte 0xff,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0x60  'a'-'f'
    .byte 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0x70
    .byte 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0x80
    .byte 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0x90
    .byte 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0xa0
    .byte 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0xb0
    .byte 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0xc0
    .byte 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0xd0
    .byte 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0xe0
    .byte 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff  // 0xf0

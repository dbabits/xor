// ARM64 (AArch64) assembly — xor_encrypt, base16encode, base16decode
// Raspberry Pi 5 (Cortex-A76, ARMv8-A)
//
// Calling convention (AAPCS64):
//   args:    x0-x7   (first 8 args; int params arrive in w0-w7)
//   scratch: x9-x15  (caller-saved, free to clobber)
//   saved:   x19-x28 (callee-saved; we don't touch them)
//   return:  x0
//   NEON:    v0-v7 caller-saved, v8-v15 callee-saved; we only use v0-v1

    .arch armv8-a
    .text

// ─────────────────────────────────────────────────────────────────────────────
// void xor_encrypt(BYTE *buffer, int bufsize, const BYTE *key, int keysize)
//   x0 = buffer   w1 = bufsize   x2 = key   w3 = keysize
//
// NEON path (when 16 % keysize == 0, e.g. keysize 1/2/4/8/16):
//   Build a 16-byte key tile by repeating the key, load into v0.
//   XOR 16 input bytes at a time with v0 — no per-byte branch needed.
//   After any multiple of 16 input bytes the key position returns to 0
//   (because 16 % keysize == 0), so the same tile stays valid forever.
//
// Scalar path (all other key lengths):
//   Classic byte-by-byte loop with conditional key-index wrap.
// ─────────────────────────────────────────────────────────────────────────────
    .global xor_encrypt
    .type   xor_encrypt, %function
xor_encrypt:
    cbz     w1, .Lxe_ret           // nothing to do if bufsize == 0
    cbz     w3, .Lxe_ret           // nothing to do if keysize == 0

    // Test whether 16 % keysize == 0 via (16/keysize)*keysize == 16
    mov     w9, #16
    udiv    w10, w9, w3             // w10 = 16 / keysize  (integer)
    mul     w10, w10, w3            // w10 = floor(16/keysize) * keysize
    cmp     w10, w9
    bne     .Lxe_scalar             // not a divisor of 16 → scalar path

    // ── NEON path ────────────────────────────────────────────────────────────
    // Allocate 16 bytes on the stack for the key tile (sp stays 16-byte aligned)
    sub     sp, sp, #16
    mov     x9, sp                  // x9 = tile write pointer
    mov     w10, #0                 // key index
    mov     w11, #16                // bytes remaining to fill
.Lxe_tile:
    ldrb    w12, [x2, w10, uxtw]   // key[key_idx]
    strb    w12, [x9], #1           // tile[i] = key[key_idx]; tile_ptr++
    add     w10, w10, #1
    cmp     w10, w3
    csel    w10, wzr, w10, eq       // wrap key index
    subs    w11, w11, #1
    bne     .Lxe_tile

    ldr     q0, [sp]                // load 16-byte tile into v0
    add     sp, sp, #16             // restore stack

    mov     x9, x0                  // x9 = data pointer
    mov     w10, w1                 // w10 = bytes remaining
.Lxe_neon16:
    cmp     w10, #16
    blt     .Lxe_neon_tail
    ldr     q1, [x9]                // load 16 data bytes
    eor     v1.16b, v1.16b, v0.16b // XOR with key tile
    str     q1, [x9], #16          // store result; advance pointer
    sub     w10, w10, #16
    b       .Lxe_neon16

    // Handle remaining < 16 bytes (key position resets to 0 after each
    // full NEON chunk because 16 % keysize == 0)
.Lxe_neon_tail:
    cbz     w10, .Lxe_ret
    mov     w11, #0                 // key index = 0 (correct after full 16-byte chunks)
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
    mov     w9,  #0                 // i (buffer index)
    mov     w10, #0                 // j (key index)
.Lxe_sloop:
    ldrb    w11, [x0, w9,  uxtw]   // buffer[i]
    ldrb    w12, [x2, w10, uxtw]   // key[j]
    eor     w11, w11, w12
    strb    w11, [x0, w9,  uxtw]   // buffer[i] ^= key[j]
    add     w9,  w9,  #1            // i++
    add     w10, w10, #1            // j++
    cmp     w10, w3
    csel    w10, wzr, w10, eq       // if j == keysize: j = 0
    cmp     w9, w1
    blt     .Lxe_sloop
.Lxe_ret:
    ret
    .size   xor_encrypt, . - xor_encrypt


// ─────────────────────────────────────────────────────────────────────────────
// void base16encode(const BYTE *buf_in, int bufinsize,
//                   char *buf_out,      int bufoutsize)
//   x0 = buf_in   w1 = bufinsize   x2 = buf_out   w3 = bufoutsize
//
// For each input byte:  buf_out[i*2] = hex[byte >> 4]
//                       buf_out[i*2+1] = hex[byte & 0xf]
// ─────────────────────────────────────────────────────────────────────────────
    .global base16encode
    .type   base16encode, %function
base16encode:
    cbz     w1, .Lbe_ret
    adr     x4, .Lhex               // x4 = "0123456789abcdef"
    mov     w5, #0                  // i
.Lbe_loop:
    ldrb    w6, [x0, w5, uxtw]     // byte = buf_in[i]
    lsr     w7, w6, #4              // hi nibble
    and     w8, w6, #0xf            // lo nibble
    ldrb    w7, [x4, w7, uxtw]     // hex[hi]
    ldrb    w8, [x4, w8, uxtw]     // hex[lo]
    lsl     w9, w5, #1              // i*2
    strb    w7, [x2, w9, uxtw]     // buf_out[i*2]   = hex[hi]
    add     w9, w9, #1
    strb    w8, [x2, w9, uxtw]     // buf_out[i*2+1] = hex[lo]
    add     w5, w5, #1
    cmp     w5, w1
    blt     .Lbe_loop
    lsl     w5, w1, #1
    strb    wzr, [x2, w5, uxtw]    // buf_out[bufinsize*2] = '\0'
.Lbe_ret:
    ret
    .size   base16encode, . - base16encode


// ─────────────────────────────────────────────────────────────────────────────
// BYTE* base16decode(const char *src, int bufinsize,
//                   BYTE *dst,        int bufoutsize)
//   x0 = src   w1 = bufinsize   x2 = dst   w3 = bufoutsize
//   returns x0 = original dst
//
// Processes pairs of hex characters; post-increments both src and dst.
// ─────────────────────────────────────────────────────────────────────────────
    .global base16decode
    .type   base16decode, %function
base16decode:
    mov     x4, x2                  // save original dst for return
    cbz     w1, .Lbd_ret
    adr     x5, .Lunhex             // x5 = unhex[256]
    lsr     w1, w1, #1              // count = bufinsize / 2
.Lbd_loop:
    ldrb    w6, [x0], #1            // hi_char = *src++
    ldrb    w7, [x0], #1            // lo_char = *src++
    ldrb    w6, [x5, w6, uxtw]     // hi = unhex[hi_char]
    ldrb    w7, [x5, w7, uxtw]     // lo = unhex[lo_char]
    lsl     w6, w6, #4              // hi << 4
    orr     w6, w6, w7              // (hi << 4) | lo
    strb    w6, [x2], #1            // *dst++ = result
    subs    w1, w1, #1
    bne     .Lbd_loop
.Lbd_ret:
    mov     x0, x4                  // return original dst
    ret
    .size   base16decode, . - base16decode


// ─────────────────────────────────────────────────────────────────────────────
// Read-only data tables
// ─────────────────────────────────────────────────────────────────────────────
    .section .rodata
    .align  4

.Lhex:
    .ascii  "0123456789abcdef"

    // unhex[c]: nibble value for ASCII hex digit c; 0xff = invalid input
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

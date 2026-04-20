// Unit tests for xor_encrypt, base16encode, base16decode.
// Compiled with -DTEST_BUILD so xor.cpp's main() is excluded.
#include "StdAfx.h"
#include <string.h>
#include <stdio.h>

// Implemented in xor.cpp (compiled alongside this file under -DTEST_BUILD)
extern void xor_encrypt(BYTE buffer[], int bufsize, const BYTE key[], int keysize);
extern void base16encode(const BYTE buffer_in[], int bufinsize, char buffer_out[], int bufoutsize);
extern BYTE* base16decode(const char base16buf[], int bufinsize, BYTE buffer_out[], int bufoutsize);

static int g_run = 0, g_fail = 0;

#define CHECK(cond) do { \
    g_run++; \
    if (!(cond)) { \
        fprintf(stderr, "FAIL [%s:%d]: %s\n", __FILE__, __LINE__, #cond); \
        g_fail++; \
    } \
} while(0)

// ─── xor_encrypt ─────────────────────────────────────────────────────────────

static void test_xor_single_byte_key() {
    BYTE buf[] = {0x41, 0x42, 0x43};
    BYTE key[] = {0xFF};
    xor_encrypt(buf, 3, key, 1);
    CHECK(buf[0] == (0x41 ^ 0xFF));
    CHECK(buf[1] == (0x42 ^ 0xFF));
    CHECK(buf[2] == (0x43 ^ 0xFF));
}

static void test_xor_key_cycling() {
    // key={1,2}, input all zeros → output repeats {1,2,1,2,1}
    BYTE buf[] = {0,0,0,0,0};
    BYTE key[] = {1,2};
    xor_encrypt(buf, 5, key, 2);
    CHECK(buf[0] == 1);
    CHECK(buf[1] == 2);
    CHECK(buf[2] == 1);
    CHECK(buf[3] == 2);
    CHECK(buf[4] == 1);
}

static void test_xor_self_inverse() {
    // encrypt(encrypt(x, k), k) == x
    BYTE original[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; // "Hello"
    BYTE buf[5];
    memcpy(buf, original, 5);
    BYTE key[] = {'p','w','d'};
    xor_encrypt(buf, 5, key, 3);
    xor_encrypt(buf, 5, key, 3);
    CHECK(memcmp(buf, original, 5) == 0);
}

static void test_xor_empty_buffer() {
    BYTE buf[] = {0x42};
    BYTE key[] = {0xFF};
    xor_encrypt(buf, 0, key, 1);
    CHECK(buf[0] == 0x42); // unchanged
}

static void test_xor_all_zero_key() {
    // XOR with 0 is identity
    BYTE buf[] = {0x01, 0x02, 0x03};
    BYTE key[] = {0x00};
    xor_encrypt(buf, 3, key, 1);
    CHECK(buf[0] == 0x01);
    CHECK(buf[1] == 0x02);
    CHECK(buf[2] == 0x03);
}

static void test_xor_all_bytes_roundtrip() {
    // All 256 byte values survive a double-encrypt round-trip
    BYTE original[256];
    for (int i = 0; i < 256; i++) original[i] = (BYTE)i;
    BYTE buf[256];
    memcpy(buf, original, 256);
    BYTE key[] = {0xAB, 0xCD, 0xEF, 0x01};
    xor_encrypt(buf, 256, key, 4);
    xor_encrypt(buf, 256, key, 4);
    CHECK(memcmp(buf, original, 256) == 0);
}

static void test_xor_key_equals_bufsize() {
    // Key exactly as long as buffer — no cycling
    BYTE buf[] = {0x00, 0x00, 0x00};
    BYTE key[] = {0x0A, 0x0B, 0x0C};
    xor_encrypt(buf, 3, key, 3);
    CHECK(buf[0] == 0x0A);
    CHECK(buf[1] == 0x0B);
    CHECK(buf[2] == 0x0C);
}

static void test_xor_multi_chunk_key_continuity() {
    // Simulate two consecutive calls — key position must reset each call.
    // Both calls independently start at key[0], so encrypting the same data
    // twice with the same key cancels out (self-inverse holds per-chunk).
    BYTE key[] = {0x11, 0x22, 0x33};
    BYTE buf[6] = {0};
    xor_encrypt(buf, 6, key, 3);
    CHECK(buf[0] == 0x11 && buf[1] == 0x22 && buf[2] == 0x33);
    CHECK(buf[3] == 0x11 && buf[4] == 0x22 && buf[5] == 0x33);
}

// ─── base16encode ─────────────────────────────────────────────────────────────

static void test_encode_known_vectors() {
    struct { BYTE in; const char* out; } cases[] = {
        {0x00, "00"}, {0xFF, "ff"}, {0x0F, "0f"}, {0xF0, "f0"},
        {0x10, "10"}, {0xA5, "a5"}, {0x5A, "5a"},
    };
    for (auto& c : cases) {
        char out[3];
        base16encode(&c.in, 1, out, 3);
        CHECK(strcmp(out, c.out) == 0);
    }
}

static void test_encode_string() {
    BYTE in[] = {'f','o','o'};
    char out[7];
    base16encode(in, 3, out, 7);
    CHECK(strcmp(out, "666f6f") == 0);
}

static void test_encode_all_256_bytes() {
    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < 256; i++) {
        BYTE in = (BYTE)i;
        char out[3];
        base16encode(&in, 1, out, 3);
        CHECK(out[0] == hex[i >> 4]);
        CHECK(out[1] == hex[i & 0xf]);
        CHECK(out[2] == '\0');
    }
}

static void test_encode_empty() {
    char out[1] = {0x7F};
    base16encode(nullptr, 0, out, 1);
    CHECK(out[0] == '\0');
}

static void test_encode_null_terminator() {
    BYTE in[] = {0xAB, 0xCD};
    char out[6];
    out[4] = 0x7F; // sentinel
    base16encode(in, 2, out, 5);
    CHECK(out[4] == '\0');
    CHECK(strcmp(out, "abcd") == 0);
}

// ─── base16decode ─────────────────────────────────────────────────────────────

static void test_decode_known_vectors() {
    struct { const char* in; BYTE out; } cases[] = {
        {"00", 0x00}, {"ff", 0xFF}, {"FF", 0xFF}, {"0f", 0x0F}, {"f0", 0xF0},
        {"Ff", 0xFF}, {"fF", 0xFF}, {"a5", 0xA5}, {"5A", 0x5A},
    };
    for (auto& c : cases) {
        BYTE out[1];
        base16decode(c.in, 2, out, 1);
        CHECK(out[0] == c.out);
    }
}

static void test_decode_multi_byte() {
    char in[] = "deadbeef";
    BYTE out[4];
    base16decode(in, 8, out, 4);
    CHECK(out[0] == 0xDE);
    CHECK(out[1] == 0xAD);
    CHECK(out[2] == 0xBE);
    CHECK(out[3] == 0xEF);
}

static void test_decode_uppercase() {
    char in[] = "DEADBEEF";
    BYTE out[4];
    base16decode(in, 8, out, 4);
    CHECK(out[0] == 0xDE && out[1] == 0xAD && out[2] == 0xBE && out[3] == 0xEF);
}

static void test_decode_returns_dst() {
    char in[] = "4142";
    BYTE out[2];
    BYTE* ret = base16decode(in, 4, out, 2);
    CHECK(ret == out);
}

static void test_decode_string() {
    char in[] = "666f6f";
    BYTE out[3];
    base16decode(in, 6, out, 3);
    CHECK(out[0] == 'f' && out[1] == 'o' && out[2] == 'o');
}

static void test_decode_roundtrip_all_256_bytes() {
    BYTE original[256];
    for (int i = 0; i < 256; i++) original[i] = (BYTE)i;

    char encoded[513];
    base16encode(original, 256, encoded, 513);

    BYTE decoded[256];
    base16decode(encoded, 512, decoded, 256);

    CHECK(memcmp(original, decoded, 256) == 0);
}

static void test_decode_empty() {
    BYTE out[1] = {0x42};
    // bufinsize=0 → assert(0%2==0) passes, loop runs 0 times
    // Note: assert(bufoutsize==bufinsize/2) requires bufoutsize=0,
    // but we pass a dummy non-null pointer; the loop is never entered.
    base16decode("", 0, out, 0);
    CHECK(out[0] == 0x42); // untouched
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main() {
    printf("Running unit tests...\n");

    test_xor_single_byte_key();
    test_xor_key_cycling();
    test_xor_self_inverse();
    test_xor_empty_buffer();
    test_xor_all_zero_key();
    test_xor_all_bytes_roundtrip();
    test_xor_key_equals_bufsize();
    test_xor_multi_chunk_key_continuity();

    test_encode_known_vectors();
    test_encode_string();
    test_encode_all_256_bytes();
    test_encode_empty();
    test_encode_null_terminator();

    test_decode_known_vectors();
    test_decode_multi_byte();
    test_decode_uppercase();
    test_decode_returns_dst();
    test_decode_string();
    test_decode_roundtrip_all_256_bytes();
    test_decode_empty();

    if (g_fail == 0) {
        printf("All %d tests passed.\n", g_run);
        return 0;
    }
    printf("%d/%d tests FAILED.\n", g_fail, g_run);
    return 1;
}

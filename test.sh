#!/bin/bash
# Integration tests for the xor utility.
# Usage: ./test.sh [path-to-binary]   (default: ./xor)
set -euo pipefail

BINARY="${1:-./xor}"
PASS=0
FAIL=0
TMPDIR_CUSTOM=$(mktemp -d)
trap 'rm -rf "$TMPDIR_CUSTOM"' EXIT

ok()  { echo "PASS: $1"; PASS=$((PASS+1)); }
fail(){ echo "FAIL: $1"; FAIL=$((FAIL+1)); }

# hex_of FILE — print file contents as lowercase hex (no spaces/newlines)
hex_of() { od -An -tx1 "$1" | tr -d ' \n'; }

check_bytes() {
    local name="$1" got="$2" expected="$3"
    if [ "$got" = "$expected" ]; then ok "$name"; else
        fail "$name (expected=$expected got=$got)"
    fi
}

# check_file NAME FILE EXPECTED_HEX
check_file() {
    local name="$1" file="$2" expected="$3"
    local got; got=$(hex_of "$file")
    if [ "$got" = "$expected" ]; then ok "$name"; else
        fail "$name (expected=$expected got=$got)"
    fi
}

# ── CLI exit codes ─────────────────────────────────────────────────────────────
"$BINARY" 2>/dev/null && fail "no-args should exit non-zero" || ok "no-args exits non-zero"
"$BINARY" -h 2>/dev/null && fail "-h should exit non-zero" || ok "-h exits non-zero"
"$BINARY" -H 2>/dev/null && fail "-H should exit non-zero" || ok "-H exits non-zero"

# empty key should be rejected
printf '' | "$BINARY" "" 2>/dev/null && fail "empty-key should exit non-zero" || ok "empty-key rejected"

# ── Empty stdin ────────────────────────────────────────────────────────────────
check_bytes "empty-stdin-xor"    "$(printf '' | "$BINARY" pwd 2>/dev/null)"         ""
check_bytes "empty-stdin-encode" "$(printf '' | "$BINARY" base16encode 2>/dev/null)" ""
check_bytes "empty-stdin-decode" "$(printf '' | "$BINARY" base16decode 2>/dev/null)" ""

# ── base16encode known vectors ─────────────────────────────────────────────────
check_bytes "encode-0x00" "$(printf '\x00' | "$BINARY" base16encode 2>/dev/null)" "00"
check_bytes "encode-0xff" "$(printf '\xff' | "$BINARY" base16encode 2>/dev/null)" "ff"
check_bytes "encode-0x0f" "$(printf '\x0f' | "$BINARY" base16encode 2>/dev/null)" "0f"
check_bytes "encode-0xf0" "$(printf '\xf0' | "$BINARY" base16encode 2>/dev/null)" "f0"
check_bytes "encode-foo"  "$(printf 'foo'  | "$BINARY" base16encode 2>/dev/null)" "666f6f"

# ── base16decode known vectors ─────────────────────────────────────────────────
check_bytes "decode-foo"     "$(printf '666f6f' | "$BINARY" base16decode 2>/dev/null)"  "foo"
printf 'DEADBEEF' | "$BINARY" base16decode 2>/dev/null > "$TMPDIR_CUSTOM/deadbeef.bin"
check_file "decode-DEADBEEF"   "$TMPDIR_CUSTOM/deadbeef.bin" "deadbeef"
printf 'DeAdBeEf' | "$BINARY" base16decode 2>/dev/null > "$TMPDIR_CUSTOM/mixedcase.bin"
check_file "decode-mixed-case" "$TMPDIR_CUSTOM/mixedcase.bin" "deadbeef"

# ── Case-insensitive keywords ──────────────────────────────────────────────────
check_bytes "encode-uppercase-keyword" "$(printf 'foo' | "$BINARY" BASE16ENCODE 2>/dev/null)" "666f6f"
check_bytes "decode-uppercase-keyword" "$(printf '666f6f' | "$BINARY" BASE16DECODE 2>/dev/null)" "foo"
check_bytes "encode-mixed-keyword"     "$(printf 'foo' | "$BINARY" Base16Encode 2>/dev/null)" "666f6f"

# ── XOR round-trip ────────────────────────────────────────────────────────────
# Use files to avoid shell null-byte swallowing of binary XOR output.
ORIG="Hello, World! This is a test of XOR encryption."
printf '%s' "$ORIG" > "$TMPDIR_CUSTOM/orig.txt"
"$BINARY" mysecretpassword 2>/dev/null < "$TMPDIR_CUSTOM/orig.txt"  > "$TMPDIR_CUSTOM/enc.bin"
"$BINARY" mysecretpassword 2>/dev/null < "$TMPDIR_CUSTOM/enc.bin"   > "$TMPDIR_CUSTOM/dec.txt"
if cmp -s "$TMPDIR_CUSTOM/orig.txt" "$TMPDIR_CUSTOM/dec.txt"; then ok "xor-roundtrip"; else fail "xor-roundtrip"; fi

# XOR with single-char key
"$BINARY" x 2>/dev/null < "$TMPDIR_CUSTOM/orig.txt"  > "$TMPDIR_CUSTOM/enc2.bin"
"$BINARY" x 2>/dev/null < "$TMPDIR_CUSTOM/enc2.bin"  > "$TMPDIR_CUSTOM/dec2.txt"
if cmp -s "$TMPDIR_CUSTOM/orig.txt" "$TMPDIR_CUSTOM/dec2.txt"; then ok "xor-roundtrip-single-char-key"; else fail "xor-roundtrip-single-char-key"; fi

# ── All 256 byte values through XOR ───────────────────────────────────────────
python3 -c "import sys; sys.stdout.buffer.write(bytes(range(256)))" \
    | "$BINARY" secret 2>/dev/null \
    | "$BINARY" secret 2>/dev/null \
    > "$TMPDIR_CUSTOM/xor256.bin"
if python3 -c "
data=open('$TMPDIR_CUSTOM/xor256.bin','rb').read()
assert data==bytes(range(256)), f'mismatch at {next(i for i,b in enumerate(data) if b!=i)}'
"; then ok "xor-all-256-bytes-roundtrip"; else fail "xor-all-256-bytes-roundtrip"; fi

# ── encode/decode round-trip for all 256 byte values ──────────────────────────
python3 -c "import sys; sys.stdout.buffer.write(bytes(range(256)))" \
    | "$BINARY" base16encode 2>/dev/null \
    | "$BINARY" base16decode 2>/dev/null \
    > "$TMPDIR_CUSTOM/enc256.bin"
if python3 -c "
data=open('$TMPDIR_CUSTOM/enc256.bin','rb').read()
assert data==bytes(range(256)), f'mismatch'
"; then ok "encode-decode-all-256-roundtrip"; else fail "encode-decode-all-256-roundtrip"; fi

# ── Full pipeline: XOR → encode → decode → XOR = original ────────────────────
"$BINARY" pwd 2>/dev/null < "$TMPDIR_CUSTOM/orig.txt" \
    | "$BINARY" base16encode 2>/dev/null \
    | "$BINARY" base16decode 2>/dev/null \
    | "$BINARY" pwd 2>/dev/null \
    > "$TMPDIR_CUSTOM/pipeline.txt"
if cmp -s "$TMPDIR_CUSTOM/orig.txt" "$TMPDIR_CUSTOM/pipeline.txt"; then ok "full-pipeline"; else fail "full-pipeline"; fi

# ── Multi-chunk (>65536 bytes): key cycling correct across fread boundaries ───
dd if=/dev/urandom of="$TMPDIR_CUSTOM/large.bin" bs=65536 count=3 2>/dev/null
cat "$TMPDIR_CUSTOM/large.bin" \
    | "$BINARY" longerkeyvalue 2>/dev/null \
    | "$BINARY" longerkeyvalue 2>/dev/null \
    > "$TMPDIR_CUSTOM/large_rt.bin"
if cmp -s "$TMPDIR_CUSTOM/large.bin" "$TMPDIR_CUSTOM/large_rt.bin"; then
    ok "xor-multi-chunk-roundtrip"
else
    fail "xor-multi-chunk-roundtrip"
fi

# ── Multi-chunk encode/decode round-trip ──────────────────────────────────────
cat "$TMPDIR_CUSTOM/large.bin" \
    | "$BINARY" base16encode 2>/dev/null \
    | "$BINARY" base16decode 2>/dev/null \
    > "$TMPDIR_CUSTOM/large_encdec.bin"
if cmp -s "$TMPDIR_CUSTOM/large.bin" "$TMPDIR_CUSTOM/large_encdec.bin"; then
    ok "encode-decode-multi-chunk-roundtrip"
else
    fail "encode-decode-multi-chunk-roundtrip"
fi

# ── Odd-length input to base16decode should fail gracefully (not abort) ────────
if printf 'f' | "$BINARY" base16decode 2>/dev/null; then
    fail "odd-input-decode-exits-nonzero"
else
    ok "odd-input-decode-exits-nonzero"
fi

# ── Binary data: encode output is printable ASCII hex ─────────────────────────
python3 -c "import sys; sys.stdout.buffer.write(bytes(range(256)))" \
    | "$BINARY" base16encode 2>/dev/null \
    > "$TMPDIR_CUSTOM/allbytes.hex"
if grep -qP '^[0-9a-f]+$' "$TMPDIR_CUSTOM/allbytes.hex"; then
    ok "encode-output-is-lowercase-hex"
else
    fail "encode-output-is-lowercase-hex"
fi

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]

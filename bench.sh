#!/bin/bash
# bench.sh <label> <results_file> [binary]
# Runs 3 trials of each operation, records median MB/s and binary size.
LABEL="$1"
OUT="$2"
BINARY="${3:-./xor}"
INPUT=bench_input.bin
HEX=bench_input.hex
INPUT_MB=100
INPUT_HEX_MB=200
RUNS=3

# Returns median elapsed milliseconds for a command (reads from stdin file)
median_ms() {
    local infile=$1; shift
    local times=()
    for i in $(seq 1 $RUNS); do
        local t0=$(date +%s%3N)
        "$@" < "$infile" > /dev/null 2>/dev/null
        local t1=$(date +%s%3N)
        times+=($((t1 - t0)))
    done
    local sorted=($(printf '%s\n' "${times[@]}" | sort -n))
    echo "${sorted[1]}"
}

mbps() {
    local mb=$1
    local ms=$2
    # mb / (ms/1000) = mb*1000/ms
    echo "scale=1; $mb * 1000 / $ms" | bc
}

echo "=== $LABEL ===" | tee -a "$OUT"

xor_ms=$(median_ms $INPUT $BINARY password)
echo "xor_mbps=$(mbps $INPUT_MB $xor_ms) (${xor_ms}ms)" | tee -a "$OUT"

enc_ms=$(median_ms $INPUT $BINARY base16encode)
echo "enc_mbps=$(mbps $INPUT_MB $enc_ms) (${enc_ms}ms)" | tee -a "$OUT"

dec_ms=$(median_ms $HEX $BINARY base16decode)
echo "dec_mbps=$(mbps $INPUT_HEX_MB $dec_ms) (${dec_ms}ms)" | tee -a "$OUT"

size=$(wc -c < $BINARY)
echo "bin_bytes=$size" | tee -a "$OUT"

echo "---" | tee -a "$OUT"

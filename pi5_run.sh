#!/bin/bash
# Run on Raspberry Pi 5: build, test, benchmark, commit results, push.
# Paste this into the Pi5 terminal and it handles everything.
set -euo pipefail

REPO_DIR="$(git rev-parse --show-toplevel)"
cd "$REPO_DIR"

echo "=== Pulling latest master ==="
git checkout master
git pull origin master

echo ""
echo "=== Building C and ASM binaries ==="
make clean
make
make asm

echo ""
echo "=== C build tests ==="
make test

echo ""
echo "=== NEON ASM tests ==="
make -C asm test

echo ""
echo "=== Generating benchmark inputs ==="
dd if=/dev/urandom of=bench_input.bin bs=1M count=100 2>/dev/null
./xor base16encode < bench_input.bin > bench_input.hex 2>/dev/null
echo "bench_input.bin and bench_input.hex ready"

echo ""
echo "=== Benchmarking C build ==="
bash bench.sh "7_tests_pi5" results.txt ./xor

echo ""
echo "=== Benchmarking NEON ASM build ==="
bash bench.sh "7_tests_pi5_asm" results.txt ./xor_asm

echo ""
echo "=== Committing and pushing results ==="
git add results.txt
git commit -m "perf: Pi5 benchmark results for commit 7_tests (C and NEON ASM)

https://claude.ai/code/session_018Q1CbLq9SURWGr7VyKKaZ2"
git push origin master

echo ""
echo "All done. Paste results.txt output below so graphs can be updated."
tail -20 results.txt

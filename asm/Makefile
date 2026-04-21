CXX      := g++
AS       := as
CXXFLAGS := -Wall -Wextra -Os -std=c++11
TARGET   := ../xor_asm
TEST_BIN := test_unit_asm

all: $(TARGET)

xor_asm.o: xor_asm.s
	$(AS) -o xor_asm.o xor_asm.s

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -I.. -c -o main.o main.cpp

$(TARGET): xor_asm.o main.o
	$(CXX) $(CXXFLAGS) -o $(TARGET) xor_asm.o main.o
	strip $(TARGET)

# Unit tests compiled against the NEON ASM implementations (ARM64 only).
$(TEST_BIN): ../test_unit.cpp xor_asm.o ../StdAfx.h
	$(CXX) $(CXXFLAGS) -I.. -DTEST_BUILD -o $@ ../test_unit.cpp xor_asm.o

# Run both unit and integration tests against the NEON ASM binary (ARM64 only).
test: $(TARGET) $(TEST_BIN)
	@echo "=== Unit tests (xor_asm NEON) ==="
	@./$(TEST_BIN)
	@echo ""
	@echo "=== Integration tests (xor_asm) ==="
	@bash ../test.sh $(TARGET)

clean:
	rm -f *.o $(TARGET) $(TEST_BIN)

.PHONY: all test clean


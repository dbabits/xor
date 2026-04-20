CXX      := g++
AS       := as
CXXFLAGS := -Wall -Wextra -Os -std=c++11
TARGET   := ../xor_asm

all: $(TARGET)

xor_asm.o: xor_asm.s
	$(AS) -o xor_asm.o xor_asm.s

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -I.. -c -o main.o main.cpp

$(TARGET): xor_asm.o main.o
	$(CXX) $(CXXFLAGS) -o $(TARGET) xor_asm.o main.o
	strip $(TARGET)

# Run integration tests against the NEON ASM binary (ARM64 only).
test: $(TARGET)
	@echo "=== Integration tests (xor_asm) ==="
	@bash ../test.sh $(TARGET)

clean:
	rm -f *.o $(TARGET)

.PHONY: all test clean

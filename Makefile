CXX      ?= g++
CXXFLAGS ?= -Wall -Wextra -Os -std=c++11
TARGET   := xor
TEST_BIN := test_unit

SRCS := xor.cpp StdAfx.cpp
OBJS := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	strip $@

%.o: %.cpp StdAfx.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(TEST_BIN): test_unit.cpp xor.cpp StdAfx.cpp StdAfx.h
	$(CXX) $(CXXFLAGS) -DTEST_BUILD -o $@ test_unit.cpp xor.cpp StdAfx.cpp

test: $(TARGET) $(TEST_BIN)
	@echo "=== Unit tests ==="
	@./$(TEST_BIN)
	@echo ""
	@echo "=== Integration tests ==="
	@bash test.sh ./$(TARGET)

asm:
	$(MAKE) -C asm

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_BIN)
	$(MAKE) -C asm clean

.PHONY: all asm test clean

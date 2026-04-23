CXX      ?= g++
CXXFLAGS ?= -Wall -Wextra -Os -std=c++11
TARGET   := xor

SRCS := xor.cpp StdAfx.cpp
OBJS := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	strip $@

%.o: %.cpp StdAfx.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

test: $(TARGET)
	@bash test.sh ./$(TARGET)

asm:
	$(MAKE) -C asm

rust:
	cd rust && cargo build --release

clean:
	rm -f $(OBJS) $(TARGET)
	$(MAKE) -C asm clean

.PHONY: all asm rust test clean

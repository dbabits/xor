CXX      ?= g++
CXXFLAGS ?= -Wall -Wextra -Os -std=c++11
TARGET   := xor

SRCS := xor.cpp StdAfx.cpp
OBJS := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp StdAfx.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

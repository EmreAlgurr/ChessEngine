CXX ?= g++
CXXFLAGS ?= -std=c++17 -O3 -march=native -mavx2 -flto -Wall -Wextra -pthread -Isrc -DUSE_AVX2=1
TARGET = ChessEngine

SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET).exe

.PHONY: all clean


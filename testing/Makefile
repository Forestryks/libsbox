CXX = g++
CXXFLAGS = --std=c++17 -Wall -O2

SRC = client.cpp test.cpp
BIN = $(SRC:%.cpp=%)

all: $(BIN)

.PHONY: clean

clean:
	rm -f $(BIN)

#CXX=clang++
#CXXFLAGS=-g -std=c++11 -Wall -pedantic
#BIN=prog

#SRC=$(wildcard *.cpp)
#OBJ=$(SRC:%.cpp=%.o)

#all: $(OBJ)
#    $(CXX) -o $(BIN) $^

#%.o: %.c
#    $(CXX) $@ -c $<

#clean:
#    rm -f *.o
#    rm $(BIN)

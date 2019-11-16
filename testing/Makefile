CXX = g++
CXXFLAGS = --std=c++17 -Wall -O2

all: client test

.PHONY: clean

clean:
	rm -f client work/test

client: client.cpp
	g++ $(CXXFLAGS) -o client client.cpp -lsbox

test: test.cpp
	g++ $(CXXFLAGS) -o work/test test.cpp

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

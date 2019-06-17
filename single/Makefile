.PHONY: all clean
.DEFAULT_GOAL := all

LIBS=-lrt -lm
INCLUDES=-I./include
#CFLAGS=-std=c++11 -g -pg -lc
CFLAGS=-O3 -std=c++11 -g -pg -lc

output = btree 

all: main

main: src/test.cpp
	g++ $(CFLAGS) -o btree src/test.cpp $(LIBS)

clean: 
	rm $(output)

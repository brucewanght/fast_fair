.PHONY: all clean
.DEFAULT_GOAL := all

LIBS=-lrt -lm -lpthread -ljemalloc
CXXFLAGS=-O3 -std=c++11 
CFLAGS=-O3 -std=gnu11 -flto -rdynamic -mtune=native -march=native -Wall -Wextra

output = btree_concurrent btree_concurrent_mixed

all: main
main: src/test.cc xxhash.o libslb.o
	g++ $(CXXFLAGS) -o btree_concurrent src/test.cc xxhash.o libslb.o $(LIBS) -DCONCURRENT
	g++ $(CXXFLAGS) -o btree_concurrent_mixed src/test.cc xxhash.o libslb.o $(LIBS) -DCONCURRENT -DMIXED

xxhash.o: src/xxhash.c
	gcc $(CFLAGS) -c src/xxhash.c

libslb.o: src/libslb.c
	gcc $(CFLAGS) -c src/libslb.c

clean: 
	rm *.o
	rm $(output)

CC=gcc
CFLAGS= -ggdb3 -Wall -Wextra -Wswitch-enum -std=c99 -pedantic

.PHONY: all clean build obj

all: mosemu mosasm mosdisasm

build:
	mkdir -p build/

obj:
	mkdir -p obj/

obj/mos.o: src/mos.c | obj
	$(CC) $(CFLAGS) -c -o $@ $<

mosemu: obj/mos.o src/mosemu.c | build
	$(CC) $(CFLAGS) -o build/$@ $^

mosasm: obj/mos.o src/mosasm.c | build
	$(CC) $(CFLAGS) -o build/$@ $^

mosdisasm: obj/mos.o src/mosdisasm.c | build
	$(CC) $(CFLAGS) -o build/$@ $^

clean:
	rm -r build/ obj/

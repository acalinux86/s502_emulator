CC=clang
CFLAGS= -ggdb3 -gdwarf-4 -O3 -Wall -Werror -Wextra -Wswitch-enum -march=native -std=c99 -pedantic

.PHONY: all clean build

all: build/mos # build/assembler

build:
	mkdir -p build/

build/mos: src/mos.c src/main.c | build
	$(CC) $(CFLAGS) -o $@ $^

build/assembler: src/assembler.c | build
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -r build/

CC=clang
CFLAGS= -Wall -Werror -Wextra -o3 -march=native -Wswitch-enum -std=c99 -pedantic -ggdb

.PHONY: all clean build

all: build/s502 build/assembler

build:
	mkdir -p build/

build/s502: src/s502.c src/main.c | build
	$(CC) $(CFLAGS) -o $@ $^

build/assembler: src/assembler.c | build
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -r build/

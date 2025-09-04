CC=clang
CFLAGS= -ggdb3 -gdwarf-4 -O3 -Wall -Werror -Wextra -Wshadow -Wswitch-enum -march=native -std=c99 -pedantic

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

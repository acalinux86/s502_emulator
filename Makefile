CC=gcc
CFLAGS= -Wall -Werror -Wextra -Wswitch-enum -std=c17 -pedantic -ggdb

.PHONY: all clean
all: build/6502emulator

build:
	mkdir -p build/

build/6502emulator: src/6502emulator.c | build
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -r build/

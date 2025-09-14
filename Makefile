CC=clang
CFLAGS= -ggdb3 -gdwarf-4 -O3 -Wall -Werror -Wextra -Wswitch-enum -march=native -std=c99 -pedantic

.PHONY: all clean build obj

all: build/mos # build/assembler

build:
	mkdir -p build/

obj:
	mkdir -p obj/

obj/mos.o: src/mos.c | obj
	$(CC) $(CFLAGS) -c -o $@ $^

build/mos: obj/mos.o src/main.c | build
	$(CC) $(CFLAGS) -o $@ $^

build/assembler: src/assembler.c | build
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -r build/ obj/

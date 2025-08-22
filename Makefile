CC=gcc
CFLAGS= -Wall -Werror -Wextra -Wswitch-enum -std=c17 -pedantic -ggdb

.PHONY: all clean
all: build/s502

build:
	mkdir -p build/

build/s502: src/s502.c src/main.c | build
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -r build/

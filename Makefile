# Makefile for the AzuDICI SAT solver
# Compiler: gcc
# Linker: ld

CC=gcc
CFLAGS=-Wall -pthread -lm -std=c99 -Wextra

all: ad

ad:
	$(CC) $(CFLAGS) src/main.c -lz -o ad

clean:
	rm -f ad

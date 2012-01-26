# Makefile for the AzuDICI SAT solver
# Compiler: gcc
# Linker: ld

CC=gcc
CFLAGS=-Wall -lpthread -lm -std=c99 -Wextra

all: ad

ad:
	$(CC) $(CFLAGS) src/main.c -o ad

clean:
	rm -f ad
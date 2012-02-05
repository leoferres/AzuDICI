# Makefile for the AzuDICI SAT solver
# Compiler: gcc
# Linker: ld

CC=gcc
CFLAGS=-Wall -pthread -lm -std=c99 -Wextra
CFILES=src/azudici.c src/clause.c src/clausedb.c src/common.c src/heap.c src/literal.c src/main.c src/model.c src/parser.c src/stats.c src/strategy.c src/test.c

all: ad

ad:
	$(CC) $(CFLAGS) src/main.c $(CFILES) -lz -o ad

clean:
	rm -f ad

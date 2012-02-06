# Makefile for the AzuDICI SAT solver
# Compiler: gcc
# Linker: ld

CC=gcc
WARNING=-Wclobbered -Wempty-body -Wignored-qualifiers -Wmissing-field-initializers -Wmissing-parameter-type -Wold-style-declaration -Woverride-init -Wtype-limits -Wuninitialized -Wunused-parameter -Wunused-but-set-parameter
CFLAGS=-Wall -pthread -std=gnu99 $(WARNING) -g
CFILES=src/azudici.c src/clause.c src/clausedb.c src/mergeSort.c src/var.c src/heap.c src/literal.c src/model.c src/stats.c src/strategy.c

all: ad

ad:
	$(CC) $(CFLAGS) src/main.c $(CFILES) -lz -lm -o ad

clean:
	rm -f ad

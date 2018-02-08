# nscp makefile

BINDIR=bin
CC=gcc
CFLAGS=-Wall -Werror
BINS=nscp

all: $(BINS)

nscp: nscp.o
	$(CC) $(CFLAGS) -o nscp nscp.o

nscp.o: nscp.c nscp.h
	$(CC) -c $(CFLAGS) nscp.c

clean:
	rm -f $(BINS) *.o

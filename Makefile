# nscp makefile

SRCDIR=src
BINDIR=bin
CC=gcc
CFLAGS=-Wall -Werror
BINS=nscp

all: $(BINS)
	rm -f *.o
	mkdir -p $(BINDIR)
	mv nscp $(BINDIR)/nscp

nscp: nscp.o
	$(CC) $(CFLAGS) -o nscp nscp.o

nscp.o: $(SRCDIR)/nscp.c $(SRCDIR)/nscp.h
	$(CC) -c $(CFLAGS) $(SRCDIR)/nscp.c $(SRCDIR)/nscp.h

clean:
	rm -f $(BINS) *.o

# nscp makefile

BINDIR=bin
CC=gcc
CFLAGS=-Wall -Werror
BINS=nscp
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

all: $(BINS)

nscp: $(OBJS)
	$(CC) $(CFLAGS) -o nscp $(OBJS)

nscp.o: nscp.c nscp.h

options.o: options.c nscp.h

clean:
	rm -f $(BINS) *.o

# Makefile for MasterMind game
# For F28HS Coursework 2

CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lm -lpthread

all: mastermind

mastermind: master-mind.o lcdBinary.o mm-matches.o
	$(CC) $(CFLAGS) -o mastermind master-mind.o lcdBinary.o mm-matches.o $(LDFLAGS)

master-mind.o: master-mind.c lcdBinary.h
	$(CC) $(CFLAGS) -c master-mind.c

lcdBinary.o: lcdBinary.c lcdBinary.h
	$(CC) $(CFLAGS) -c lcdBinary.c

mm-matches.o: mm-matches.s
	as -o mm-matches.o mm-matches.s

clean:
	rm -f mastermind *.o

run: mastermind
	sudo ./mastermind

debug: mastermind
	sudo gdb ./mastermind
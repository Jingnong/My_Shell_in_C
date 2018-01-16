CFLAGS=-g -pedantic -std=c99 -Wall# -O2
CC=gcc

all: myshell

myshell: myshell.o util.o
	$(CC) $(CFLAGS) -o mysh myshell.o util.o

util.o: util.c util.h
	$(CC) $(CFLAGS) -o util.o -c util.c

run_shell.o: myshell.c myshell.h util.h
	$(CC) $(CFLAGS) -o myshell.o -c myshell.c

test: myshell_rl
	./myshell

clean:
	rm -f *.o myshell myshell

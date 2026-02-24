# Makefile para o Compilador C-

CC = gcc
CFLAGS = -Wall -g

OBJS = main.o util.o scan.o parse.o symtab.o analyze.o cgen.o

cminus: $(OBJS)
	$(CC) $(CFLAGS) -o cminus $(OBJS)

main.o: main.c globals.h util.h scan.h parse.h analyze.h symtab.h cgen.h
	$(CC) $(CFLAGS) -c main.c

util.o: util.c util.h globals.h
	$(CC) $(CFLAGS) -c util.c

scan.o: scan.c scan.h util.h globals.h
	$(CC) $(CFLAGS) -c scan.c

parse.o: parse.c parse.h scan.h globals.h util.h
	$(CC) $(CFLAGS) -c parse.c

symtab.o: symtab.c symtab.h globals.h
	$(CC) $(CFLAGS) -c symtab.c

analyze.o: analyze.c analyze.h globals.h symtab.h
	$(CC) $(CFLAGS) -c analyze.c

cgen.o: cgen.c cgen.h globals.h
	$(CC) $(CFLAGS) -c cgen.c

clean:
	rm -f cminus $(OBJS)

test: cminus
	./cminus test.cm
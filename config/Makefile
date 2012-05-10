CC=gcc
CFLAGS=-c -Wall
OBJFLAGS=-o
DEBUG=-g

test:cfg.o
	$(CC) $(OBJFLAGS) $@ test.c cfg.o

cfg.o:cfg.c
	$(CC) $(CFLAGS) cfg.c

clean:
	rm -f *.o test




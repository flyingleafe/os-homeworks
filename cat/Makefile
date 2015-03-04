CC=gcc
LIBDIR=../lib
CFLAGS=-I$(LIBDIR)
LDFLAGS=-L$(LIBDIR) -lhelpers

all: cat

cat: cat.o libhelpers
	$(CC) -o $@ $(LDFLAGS) cat.o

cat.o:
	$(CC) -o $@ -c $(CFLAGS) cat.c 

libhelpers:
	$(MAKE) -C $(LIBDIR)

clean:
	rm -f cat cat.o

CC=gcc
LIBDIR=../lib
CFLAGS=-I$(LIBDIR) -ggdb
LDFLAGS=-L$(LIBDIR) -lhelpers
PROGRAM=simplesh
UNAME=$(shell uname -s)

ifeq ($(UNAME), Linux)
	LDFLAGS += -Wl,-rpath,$(LIBDIR)
endif

all: $(PROGRAM)

$(PROGRAM): $(PROGRAM).o libhelpers
	$(CC) -o $@ $< $(LDFLAGS)
$(PROGRAM).o:
	$(CC) -o $@ -c $(CFLAGS) $(PROGRAM).c

libhelpers:
	$(MAKE) -C $(LIBDIR)

clean:
	rm -f $(PROGRAM) $(PROGRAM).o
	$(MAKE) -C $(LIBDIR) clean

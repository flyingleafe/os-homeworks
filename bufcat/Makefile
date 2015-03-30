PROGRAM=bufcat
LIBS=bufio helpers

CC=gcc
LIBDIR=../lib
CFLAGS=-I$(LIBDIR)
UNAME=$(shell uname -s)
LDFLAGS=-L$(LIBDIR) $(foreach lib, $(LIBS),-l$(lib) )

ifeq ($(UNAME), Linux)
	LDFLAGS += -Wl,-rpath,$(LIBDIR)
endif

.PHONY: all
all: $(PROGRAM)

$(PROGRAM): $(PROGRAM).o libs
	$(CC) -o $@ $< $(LDFLAGS)

$(PROGRAM).o:
	$(CC) -o $@ -c $(CFLAGS) $(PROGRAM).c

libs:
	$(MAKE) -C $(LIBDIR)

.PHONY: clean
clean:
	rm -f $(PROGRAM) $(PROGRAM).o
	$(MAKE) -C $(LIBDIR) clean

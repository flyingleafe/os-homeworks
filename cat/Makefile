CC = gcc
LIBDIR = ../lib
CFLAGS = -I$(LIBDIR)
UNAME = $(shell uname -s)
LDFLAGS = -L$(LIBDIR) -lhelpers
PROGRAM = cat

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

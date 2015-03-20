CC=gcc
LIBDIR=$(shell pwd)
CFLAGS=-Wall -fPIC
LDFLAGS=
LIBRARY=helpers
UNAME = $(shell uname -s)

ifeq ($(UNAME), Darwin)
	LDFLAGS += -install_name $(LIBDIR)/lib$(LIBRARY).so
endif

all: lib$(LIBRARY).so

lib$(LIBRARY).so: $(LIBRARY).o
	$(CC) -o $@ $(LDFLAGS) -shared $<

$(LIBRARY).o:
	$(CC) -o $@ -c $(CFLAGS) $(LIBRARY).c

clean:
	rm -f lib$(LIBRARY).so $(LIBRARY).o

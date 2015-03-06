CC=gcc
LIBDIR=.
CFLAGS=-Wall -fPIC
LDFLAGS=
LIBRARY=helpers

all: lib$(LIBRARY).so

lib$(LIBRARY).so: $(LIBRARY).o
	$(CC) -o $@ $(LDFLAGS) -shared $<

$(LIBRARY).o:
	$(CC) -o $@ -c $(CFLAGS) $(LIBRARY).c

clean:
	rm -f lib$(LIBRARY).so $(LIBRARY).o

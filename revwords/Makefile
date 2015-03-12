CC=gcc
LIBDIR=../lib
CFLAGS=-I$(LIBDIR) -ggdb
LDFLAGS=-Wl,-rpath,$(LIBDIR) -L$(LIBDIR) -lhelpers
PROGRAM=revwords

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

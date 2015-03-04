all: libhelpers.so

libhelpers.so: libhelpers.o
	gcc -o libhelpers.so -shared libhelpers.o

libhelpers.o:
	gcc -o libhelpers.o -c -fPIC helpers.c

clean:
	rm -f libhelpers.o libhelpers.so

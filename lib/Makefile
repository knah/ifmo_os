all: libhelpers.so

libhelpers.so: helpers.o
	gcc -shared helpers.o -o libhelpers.so

helpers.o:
	gcc -c -fPIC -std=c99 helpers.c -o helpers.o

clean:
	rm -f helpers.o
	rm -f libhelpers.so
all: libhelpers.so libbufio.so
CFLAGS=-g

libhelpers.so: helpers.o
	gcc $(CFLAGS) -shared helpers.o -o libhelpers.so

libbufio.so: bufio.c
	gcc $(CFLAGS) -shared -fPIC -std=c99 bufio.c -o libbufio.so

helpers.o: helpers.c
	gcc $(CFLAGS) -c -fPIC -std=c99 helpers.c -o helpers.o

clean:
	rm -f helpers.o
	rm -f libhelpers.so
	rm -f libbufio.so
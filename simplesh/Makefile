all: simplesh

simplesh: simplesh.c
	gcc -g -std=c99 -I../lib -L../lib -o simplesh simplesh.c -lhelpers -lbufio

clean:
	rm -f ./simplesh
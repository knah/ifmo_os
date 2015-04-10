all: foreach

foreach: foreach.c
	gcc -std=c99 -I../lib -L../lib -o foreach foreach.c -lhelpers -lbufio

clean:
	rm -f ./foreach

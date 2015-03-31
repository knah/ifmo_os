all: filter

filter: filter.c
	gcc -std=c99 -I../lib -L../lib -o filter filter.c -lhelpers

clean:
	rm -f ./filter

all: cat

cat: cat.c
	gcc -std=c99 -I../lib -L../lib -o cat cat.c -lhelpers

clean:
	rm -f ./cat
all: bufcat

bufcat: bufcat.c
	gcc -std=c99 -I../lib -L../lib -o bufcat bufcat.c -lbufio

clean:
	rm -f ./bufcat
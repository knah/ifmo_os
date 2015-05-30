all: filesender

filesender: filesender.c
	gcc -g -std=c99 -I../lib -L../lib -o filesender filesender.c -lhelpers -lbufio

clean:
	rm -f ./filesender
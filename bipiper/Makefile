all: forking

forking: forking.c
	gcc -g -std=c99 -I../lib -L../lib -o forking forking.c -lhelpers -lbufio

clean:
	rm -f ./forking
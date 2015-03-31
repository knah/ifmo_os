all: revwords

revwords: revwords.c
	gcc -std=c99 -I../lib -L../lib -o revwords revwords.c -lhelpers

clean:
	rm -f ./revwords
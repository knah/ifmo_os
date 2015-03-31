all: delwords

delwords: delwords.c
	gcc -std=c99 -I../lib -L../lib -o delwords delwords.c -lhelpers

clean:
	rm -f ./delwords
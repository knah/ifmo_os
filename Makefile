all: lib/libhelpers.so cat/cat revwords/revwords

lib/libhelpers.so:
	cd lib && make

cat/cat:
	cd cat && make

revwords/revwords:
	cd revwords && make
	


clean:
	cd cat && make clean
	cd lib && make clean
	cd revwords && make clean
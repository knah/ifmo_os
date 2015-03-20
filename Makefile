all: lib/libhelpers.so cat/cat revwords/revwords filter/filter

lib/libhelpers.so:
	cd lib && make

cat/cat:
	cd cat && make

revwords/revwords:
	cd revwords && make
	
filter/filter:
	cd filter && make


clean:
	cd cat && make clean
	cd lib && make clean
	cd revwords && make clean
	cd filter && make clean
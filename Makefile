all: lib/libhelpers.so cat/cat

lib/libhelpers.so:
	cd lib && make

cat/cat:
	cd cat && make

clean:
	cd cat && make clean
	cd lib && make clean
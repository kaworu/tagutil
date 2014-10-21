.PHONY: all debug test clean

all:
	test -d build || mkdir build
	cd build && cmake ../src && make

debug:
	test -d build || mkdir build
	cd build && CFLAGS="-g -Wall -O0" cmake -DCMAKE_VERBOSE_MAKEFILE=YES ../src && make

test:
	make -C test

clean:
	rm -rf build

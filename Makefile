all:
	[ -d build ] || mkdir build
	cd build && cmake .. && make

debug:
	[ -d build ] || mkdir build
	cd build && CFLAGS="-g -Wall -O0" cmake -DCMAKE_VERBOSE_MAKEFILE=YES .. && make

test:
	make -C test

clean:
	rm -rf build

.PHONY: all debug test clean

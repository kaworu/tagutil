BUILD_TYPE?=DEBUG

all: build
	cd build && cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_VERBOSE_MAKEFILE=YES ../src && make

build:
	mkdir build

test:
	make -C test

clean:
	rm -rf build

.PHONY: all release debug grim test clean

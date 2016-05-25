.PHONY: all release debug grim _mkdir_build test clean

all: release

release: _mkdir_build
	cd build && cmake -DCMAKE_BUILD_TYPE=RELEASE ../src && make

debug: _mkdir_build
	cd build && cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_VERBOSE_MAKEFILE=YES ../src && make

grim: _mkdir_build
	cd build && cmake -DCMAKE_BUILD_TYPE=GRIM -DCMAKE_VERBOSE_MAKEFILE=YES ../src && make

_mkdir_build:
	test -d build || mkdir build

test:
	make -C test

clean:
	rm -rf build

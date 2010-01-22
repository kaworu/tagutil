all:
	[ -d build ] || mkdir build
	cd build && cmake .. && make

debug:
	[ -d build ] || mkdir build
	cd build && CC=clang cmake -DCMAKE_VERBOSE_MAKEFILE=YES .. && make

clean:
	rm -rf build

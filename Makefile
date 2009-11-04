all:
	[ -d build ] || mkdir build
	cd build && cmake .. && make

debug:
	[ -d build ] || mkdir build
	cd build && cmake -DCMAKE_VERBOSE_MAKEFILE=YES .. && make

clean:
	rm -rf build

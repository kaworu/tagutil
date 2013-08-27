#Installation

## Required Dependencies:

- pkg-config (build dep)
- cmake >= 2.6 (build dep)
- libyaml


## Optionals Dependencies:

Optionals dependencies are detected by cmake. If you want to override the
detection you can define WITHOUT_<DEP> to avoid tagutil to link against
DEP.

- FLAC: libFLAC
    If you want the flac files to be handled by libFLAC.
- OGGVORBIS: libvorbis
    If you want the ogg/vorbis files to be handled by libvorbis.
    XXX: write support is not available atm.
- TAGLIB: TagLib (>=1.5)
    generic backend. Can handle a lot of different file type, but only a
    limited set of tags (artist, title, album, tracknumber, date, genre and
    comment)


## Building:

type `make', it'll create the build/ directory and call cmake and make to
configure and build tagutil. Or if you want to gives some arguments to
cmake do (for example):
```
mkdir build && cd build
cmake -DWITHOUT_OGGVORBIS
make
```

## Install:

build tagutil, then `cd build && make install'. You might need root access
to call `make install'.

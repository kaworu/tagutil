#Installation

## Required Dependencies:

- pkg-config (build dep)
- cmake >= 2.6 (build dep)
- libyaml


## Optionals Dependencies:

Optionals dependencies are detected by cmake. If you want to override the
detection you can define `WITHOUT_$LIB` to avoid tagutil to link against
`$LIB`.

- JSON: jansson
    JSON output format.
- FLAC: libFLAC
    If you want the flac files to be handled by libFLAC.
- OGGVORBIS: libvorbis
    If you want the ogg/vorbis files to be handled by libvorbis.
- TAGLIB: TagLib (>=1.5)
    Generic backend. Can handle a lot of different file type, but only a
    limited set of tags (artist, title, album, tracknumber, date, genre and
    comment).
- ID3V1:
    A stock ID3v1.1 TAG backend. ID3v1 is only used by very old mp3 files and
    has a lot of limitation including: limited set of tags, limited length (30
    characters at the most), genre has to be part of the ID3v1 list (this
    backend support the Winamp extended list though) etc.


## Building:

type `make`, it'll create the build/ directory and call cmake and make to
configure and build tagutil. Or if you want to gives some arguments to
cmake do (for example):
```
mkdir build && cd build
cmake -DWITHOUT_TAGLIB=yes ..
make
```

## Install:

build tagutil, then `cd build && make install`. You might need root access
to call `make install`.

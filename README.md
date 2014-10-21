Tagutil - CLI music files tags editor
=====================================

`tagutil(1)` is a CLI tool to edit music file's tag. It aims to provide both an
easy-to-script interface and ease of use interactively. Unlike most other tag
tools out there, it fully support _Vorbis Comments_ for both ogg/vorbis files
and FLAC. _Vorbis Comments_ are sexy, because you can set tags with any
key/value without much restrictions (you can even have more than one value for
a key).

Installation
============

Required Dependencies:
----------------------

- pkg-config (build dep)
- cmake >= 2.6 (build dep)
- libyaml

Optionals Dependencies:
----------------------

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


Building:
---------

type `make`, it'll create the build/ directory and call cmake and make to
configure and build tagutil. Or if you want to gives some arguments to
cmake do (for example):
```
mkdir build && cd build
cmake -DWITHOUT_TAGLIB=yes ..
make
```

## Installation:
----------

### From Source:

build tagutil, then `cd build && make install`. You might need root access
to call `make install`.

### From Arch Linux:

A [package](https://aur.archlinux.org/packages/tagutil/) is provided in the
[AUR](https://wiki.archlinux.org/index.php/AUR). Use your favorite [AUR
helper](https://wiki.archlinux.org/index.php/AUR_helpers)
to install it or download the AUR tarball and run `makepkg -si` to build and
install the package.

Usage
=====

reading tags
------------
Use the **print** action. Since it is the default action, you can omit it from
the command line:

```
% tagutil fearless.flac
# fearless.flac
---
- album: Meddle
- artist: Pink Floyd
- date: 1971
- title: Fearless
- tracknumber: 03
- genre: Progressive Rock
```

editing tags
------------
There are commands for basic editing stuff like **clear**, **add** and **set**.
If you need to do something more complex, the **edit** action let you use your
favourite `$EDITOR`.

renaming files
--------------
One powerful feature of **tagutil** is the **rename** action. It rename the
music files after inferring the new name from a rename pattern. In the rename
pattern, `%{name}` is replaced by the `name` tag. You can also use the simpler
form `%name` if the tag is only composed of alphanumeric characters. Example:

```
% tagutil -p rename:"[%date] %artist/%tracknumber - %title" fearless.flac
rename `fearless.flac' to `[1971] Pink Floyd/03 - Fearless.flac'? [y/n]
```

scripting
---------
**tagutil** can easily be scripted. Basic scripts can use the editing actions
while more complex scripts can use **print**, parse the output, do some
modifications and then use **load**. There are two examples in the _scripts/_
directory:

* _scripts/tagutil-track_: this simple Perl script will take a tag name and
  some files as arguments, and will set `01` for the first file, `02` for the
  second and so on. Useful to set the track number of an album.
* _scripts/tagutil-trim_: this Ruby script is a bit more complex and is an
  example using YAML parsing. What is does is very simple though, it just trim
  every tags of leading and trailing white space(s).

full --help
-----------

```
% tagutil -h
tagutil v3.0

usage: tagutil [OPTION]... [ACTION:ARG]... [FILE]...
Modify or display music file's tag.

Options:
  -h     show this help
  -p     create destination directories if needed (used by rename)
  -F fmt use the fmt format for print, edit and load actions (see Formats)
  -Y     answer yes to all questions
  -N     answer no  to all questions

Actions:
  print            print tags (default action)
  backend          print the backend used (see Backend)
  clear:TAG        clear all tag TAG. If TAG is empty, all tags are cleared
  add:TAG=VALUE    add a TAG=VALUE pair
  set:TAG=VALUE    set TAG to VALUE
  edit             prompt for editing
  load:PATH        load PATH yaml tag file
  rename:PATTERN   rename to PATTERN

Formats:
         yml: YAML - YAML Ain't Markup Language
        json: JSON - JavaScript Object Notation

Backends:
     libFLAC: Free Lossless Audio Codec (FLAC) files format
   libvorbis: Ogg/Vorbis files format
      TagLib: various file format but limited set of tags
```

Test
====
```
% gem install bundler
```

LICENSE
=======
It is a BSD 2-Clause license, see LICENSE.

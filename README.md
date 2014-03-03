Tagutil - CLI music files tags editor
=====================================

`tagutil(1)` is a CLI tool to edit music file's tag. It aim to provide both an
easy-to-script interface and ease of use interactively. Unlike most other tag
tools out there, it fully support _Vorbis Comments_ for both ogg/vorbis files
and FLAC. _Vorbis Comments_ are sexy, because you can set tags with any
key/value without much restrictions (you can even have more than one value for
a key).

Installation
============
**tagutil** use CMake for build and installation. See the INSTALL file.

Usage
=====

--help
------

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
If you need to do something more complex, the `edit` action let you use your
favourite `$EDITOR`.

renaming files
--------------
One powerful feature of **tagutil** is the **rename** action. It rename the
music files after deferring the new name from a _rename pattern_. In the
_rename pattern_ `%{name}` is replaced by the `name`tag. You can also use the
simpler `%name` (in that case the delimiter is a space). Example:

```
% tagutil -p rename:"[%{date}] %{artist}/%tracknumber - %title" fearless.flac
rename `fearless.flac' to `[1971] Pink Floyd/03 - Fearless.flac'? [y/n]
```

scripting
---------
**tagutil** can easily be scripted. Basic scripts can use the editing actions
while more complex scripts can use **print**, parse the output, do some
modifications and then use **load**. There are two examples in the _scripts/_
directory:

* tagutil-track: this simple Perl script will take a tag name and some files as
  arguments, and will set `01` for the first file, `02` for the second and so
  on. Useful to set the track number of an album.
* tagutil-trim: this Ruby script is a bit more complex and is an example using
  YAML parsing. What is does is very simple though, it just trim every tags of
  leading and trailing white space(s).


LICENSE
=======
It is a BSD 2-Clause license, see LICENSE.

# BSD Makefile
.sinclude "config.mk"

PROG=tagutil
SRCS=tagutil.c \
	 t_toolkit.c \
	 t_tag.c \
	 t_yaml.c \
	 t_renamer.c \
	 t_lexer.c \
	 t_parser.c \
	 t_filter.c

VERSION=2.1


# install options
DESTDIR?=/usr/local
BINDIR?=/bin
MANDIR?=/man/man


# C opts
CSTD=c99
.if defined(DEBUG)
CFLAGS=-g -O0 -Wall -Wsystem-headers -Wno-format-y2k -W -Wno-unused-parameter \
	-Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith -Wreturn-type \
	-Wwrite-strings -Wswitch -Wshadow -Wcast-align -Wunused-parameter \
	-Wchar-subscripts -Winline -Wnested-externs -Wredundant-decls -Wno-pointer-sign
VERSION:=${VERSION}-debug
.endif
CFLAGS+=-I. -D'T_TAGUTIL_VERSION="${VERSION}"'
CFLAGS+=	-DHAS_GETPROGNAME \
			-DHAS_STRLCPY \
			-DHAS_STRLCAT \
			-DHAS_STRDUP \
			-DHAS_SYS_QUEUE_H

# libyaml doesn't use pkg-config :(
CFLAGS+=-I/usr/local/include
LDADD+=-L/usr/local/lib -lyaml

# Generic support with TagLib
.if defined(WITH_TAGLIB)
SRCS+=t_ftgeneric.c
TAGLIB_I!=pkg-config --cflags taglib_c
CFLAGS+=-DWITH_TAGLIB ${TAGLIB_I}
TAGLIB_L!=pkg-config --libs   taglib_c
LDADD+=${TAGLIB_L}
.endif

# FLAC support with libFLAC
.if defined(WITH_FLAC)
SRCS+=t_ftflac.c
FLAC_I!=pkg-config --cflags flac
# use -iquote instead of -I to avoid a assert.h clash
CFLAGS+=-DWITH_FLAC ${FLAC_I:S/-I/-iquote /}
FLAC_L!=pkg-config --libs   flac
LDADD+=${FLAC_L}
.endif

# Ogg/Vorbis support with libvorbis
.if defined(WITH_OGGVORBIS)
SRCS+=t_ftoggvorbis.c
OGGVORBIS_I!=pkg-config --cflags vorbisfile
CFLAGS+=-DWITH_OGGVORBIS ${OGGVORBIS_I}
OGGVORBIS_L!=pkg-config --libs   vorbisfile
LDADD+=${OGGVORBIS_L}
.endif


.include <bsd.prog.mk>

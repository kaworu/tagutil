# BSD Makefile

PROG=tagutil
SRCS=tagutil.c t_lexer.c t_parser.c t_interpreter.c t_yaml.c t_renamer.c

VERSION=2.1


# install options
DESTDIR?=/usr/local
BINDIR?=/bin
MANDIR?=/man/man


# C opts
CSTD=c99
.if defined(DEBUG)
CFLAGS=-g -O0 -Wall -Wextra -Wformat-security -Wnonnull -Waggregate-return \
	   -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls -Wshadow \
	   -Wstrict-prototypes -Winline -Wall
VERSION+=(debug)
.endif
CFLAGS+=-D'VERSION="${VERSION}"' -DHAVE_SANE_DIRNAME

# libyaml can't use pkg-config :(
CFLAGS+=-I/usr/local/include
LDFLAGS+=-L/usr/local/lib -lyaml

# Generic support with TagLib
.if defined(WITH_TAGLIB)
SRCS+=t_ftgeneric.c
TAGLIB_I!=pkg-config --cflags taglib_c
CFLAGS+=-DWITH_TAGLIB ${TAGLIB_I}
TAGLIB_L!=pkg-config --libs   taglib_c
LDFLAGS+=${TAGLIB_L}
.endif

# FLAC support with libFLAC
.if defined(WITH_FLAC)
SRCS+=t_ftflac.c
FLAC_I!=pkg-config --cflags flac
CFLAGS+=-DWITH_FLAC ${FLAC_I}
FLAC_L!=pkg-config --libs   flac
LDFLAGS+=${FLAC_L}
.endif


.include <bsd.prog.mk>

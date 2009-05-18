# BSD Makefile

PROG=tagutil
SRCS=tagutil.c t_ftgeneric.c t_ftflac.c t_lexer.c t_parser.c t_interpreter.c t_yaml.c t_renamer.c

VERSION=2.1

# install options
DESTDIR?=/usr/local
BINDIR?=/bin
MANDIR?=/man/man

# TagLib
TAGLIB_I!=pkg-config --cflags taglib_c
TAGLIB_L!=pkg-config --libs   taglib_c

# libFLAC
FLAC_I!=pkg-config --cflags flac
FLAC_L!=pkg-config --libs   flac


# C opts
CSTD=c99
.if defined(DEBUG)
CFLAGS=-g -O0 -Wall -Wextra -Wformat-security -Wnonnull -Wswitch-default -Waggregate-return -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls -Wshadow -Wstrict-prototypes -Winline -Wall
VERSION+=(debug)
.endif
CFLAGS+=-D'VERSION="${VERSION}"' \
		-DHAVE_SANE_DIRNAME \
		${TAGLIB_I} \
		#${FLAC_I}

LDFLAGS+=${TAGLIB_L}
LDFLAGS+=${FLAC_L}


.include <bsd.prog.mk>

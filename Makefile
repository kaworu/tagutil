# BSD Makefile
CSTD=c99
PROG=tagutil
SRCS=tagutil.c t_lexer.c t_parser.c t_interpreter.c t_yaml.c t_renamer.c
VERSION=2.1
AUTHORS=kAworu
DESTDIR?=/usr/local
BINDIR?=/bin
MANDIR?=/man/man

.if defined(DEBUG)
CFLAGS=-g -O0 -Wall -Wextra -Wformat-security -Wnonnull -Wswitch-default -Waggregate-return -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls -Wshadow -Wstrict-prototypes -Winline -Wall
VERSION+=(debug)
.endif
CFLAGS+=-D__TAGUTIL_VERSION__="\"${VERSION}\"" \
		-D__TAGUTIL_AUTHORS__="\"${AUTHORS}\"" \
		-DHAVE_SANE_DIRNAME \
		`pkg-config --cflags taglib_c`
LDFLAGS+=`pkg-config --libs taglib_c`

.include <bsd.prog.mk>

depends:
	@${CC} ${CFLAGS} -E -MM ${SRCS} > .depend

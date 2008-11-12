PROG		= tagutil
SRCS		= tagutil.c t_toolkit.c t_lexer.c t_parser.c t_interpreter.c
CFLAGS		+= -Wall -I/usr/local/include/
CSTD		?= c99
LDADD		= -L/usr/local/lib -ltag_c

.if defined(WITH_MUSICBRAINZ)
SRCS		+= t_xml.c t_http.c t_mb.c
#DPADD		= ${LIBEXPAT}
LDADD		+= -lexpat
.endif

.if defined(DEBUG)
CFLAGS	= -std=iso9899:1999 -g -Wextra -Wformat-security -Wnonnull -Wswitch-default -Waggregate-return -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls -Wshadow -Wstrict-prototypes -Winline -Wall -I/usr/local/include/
.endif

depend:
	: > .depend
.for s in ${SRCS}
	gcc -E -MM ${s} >> .depend
.endfor

.include <bsd.prog.mk>

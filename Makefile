PROG		= tagutil
SRCS		= tagutil.c t_toolkit.c
CFLAGS		+= -Wall -I/usr/local/include/
CSTD		?= c99
LDADD		= -L/usr/local/lib -ltag_c

.if defined(WITH_MUSICBRAINZ)
SRCS		+= t_xml.c t_http.c t_mb.c
#DPADD		= ${LIBEXPAT}
LDADD		+= -lexpat
.endif

.if defined(DEBUG)
DEBUG_FLAGS	= -g -Wextra -Wformat-security -Wnonnull -Wswitch-default -Wswitch-enum -Waggregate-return -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls -Wshadow -Wstrict-prototypes -Winline
.endif

depend:
	: > ../.depend
.for s in ${SRCS}
	gcc -E -MM ../${s} >> ../.depend
.endfor

.include <bsd.prog.mk>

PROG		 = tagutil
SRCS		 = tagutil.c t_toolkit.c t_lexer.c t_parser.c t_interpreter.c

PREFIX		?= /usr/local
BINDIR		?= /bin
DESTDIR		 = ${PREFIX}

CFLAGS		+= -Wall -I/usr/local/include/
CSTD		 = c99
LDADD		 = -L/usr/local/lib -ltag_c

depends:
	@: > .depend
.for s in ${SRCS}
	@${CC} -E -MM ${s} >> .depend
.endfor

.include <bsd.prog.mk>

PROG		 = tagutil
SRCS		 = tagutil.c t_toolkit.c t_lexer.c t_parser.c t_interpreter.c

PREFIX		?= /usr/local
BINDIR		?= /bin
DESTDIR		 = ${PREFIX}

CSTD		 = c99
LDADD		 = -L/usr/local/lib -ltag_c

CFLAGS		+= -Wall -I/usr/local/include/

depends:
	@${CC} -E -MM ${SRCS} > .depend

.include <bsd.prog.mk>

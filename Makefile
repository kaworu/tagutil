PROG		= tagutil
SRCS		= tagutil.c t_toolkit.c t_xml.c t_http.c t_mb.c
CFLAGS		= -std=c99 -Wall -I/usr/local/include/ # -O2 -pipe
LDFLAGS		= -L/usr/local/lib -lexpat -ltag_c
DEBUG_FLAGS	= -g -Wextra -Wformat-security -Wnonnull -Wswitch-default -Wswitch-enum -Waggregate-return -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls -Wshadow -Wstrict-prototypes -Winline

.include <bsd.prog.mk>

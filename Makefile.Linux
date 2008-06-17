PROG = tagutil

CFLAGS  += -g -Wall -std=c89 -pedantic -D_XOPEN_SOURCE=500 # dev flags
#CFLAGS  += -std=c99 -Wall -O2 -pipe -D_XOPEN_SOURCE=500
LDFLAGS += -ltag_c -lexpat

clean:
	@rm $(PROG) *.o &>/dev/null || true
tags: $(PROG).c
	ctags *.c
tagutil: t_xml.o t_http.o t_toolkit.o tagutil.o
	cc -o $(PROG) $(CFLAGS) $(LDFLAGS) $+

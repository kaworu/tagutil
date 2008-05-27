#CFLAGS += -std=c89 -pedantic -I/usr/include/taglib -D_XOPEN_SOURCE=500 # dev flags
CFLAGS += -std=c99 -O2 -I/usr/include/taglib -D_XOPEN_SOURCE=500
LDFLAGS=-ltag_c

clean:
	@rm tagutil &>/dev/null || true
tags: *.c
	ctags tagutil.c

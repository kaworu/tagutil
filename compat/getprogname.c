#include <stdlib.h>

extern char *__progname;

const char *
getprogname(void)
{

	return (__progname);
}

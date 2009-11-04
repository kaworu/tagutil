/*
 * build-tests/i_can_haz_getprogname.c
 */
#include <stdlib.h>

int
main(void)
{
	const char *_ = getprogname();
	return (0);
}


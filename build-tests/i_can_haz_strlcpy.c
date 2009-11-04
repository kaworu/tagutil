/*
 * build-tests/i_can_haz_strlcpy.c
 */
#include <string.h>

int
main(void)
{
	const char *orig = "foo!";
	char buf[42];

	(void)strlcpy(buf, orig, sizeof(buf));
	/* check? */
	return (0);
}


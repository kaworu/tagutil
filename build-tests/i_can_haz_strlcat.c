/*
 * build-tests/i_can_haz_strlcat.c
 */
#include <string.h>

int
main(void)
{
	const char *begin = "begin";
	const char *end   = "end";
	char buf[42];

	buf[0] = '\0';
	(void)strlcat(buf, begin, sizeof(buf));
	(void)strlcat(buf, end,   sizeof(buf));
	/* check? */
	return (0);
}

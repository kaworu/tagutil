/*
 * build-tests/i_can_haz_strdup.c
 */
#include <string.h>

int
main(void)
{
	const char *str = "foo";
	const char *cpy = strdup(str);
	/* check? */
	if (cpy != NULL)
		free(cpy);
	return (0);
}

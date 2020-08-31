/*
 * tests/i_can_haz_eaccess.c
 */
#include <unistd.h>

int
main(void)
{
	eaccess("", 0);
}

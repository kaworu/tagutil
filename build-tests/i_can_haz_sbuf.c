/*
 * build-tests/i_can_haz_sbuf.c
 */
#include <stddef.h>
#include <sys/types.h>
#include <sys/sbuf.h>

int
main(void)
{
	struct sbuf *_ = sbuf_new_auto();
	(void)_;

	return (0);
}

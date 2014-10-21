/*
 * tests/i_can_haz_sbuf.c
 */
#include <stddef.h>
#include <sys/types.h>
#include <sys/sbuf.h>

int
main(void)
{
	struct sbuf *_ = sbuf_new_auto();

	/*
	 * XXX: DragonFly has a different interface. where sbuf_finish() return
	 * void.
	 */
	if (sbuf_finish(_) == -1)
		return (-1);

	return (0);
}

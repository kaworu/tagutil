/*
 * build-tests/i_can_haz_sys_queue_h.c
 */
#include <sys/param.h>
#include <sys/queue.h>

struct e {
	const char *txt;
	STAILQ_ENTRY(e) next;
};


int
main(void)
{
	STAILQ_HEAD(, e) head = STAILQ_HEAD_INITIALIZER(head);
	struct e a, b;

	a.txt = "foo";
	b.txt = "bar";
	STAILQ_INSERT_HEAD(&head, &b, next);
	STAILQ_INSERT_HEAD(&head, &a, next);

	/* test STAILQ_LAST for __offsetof, GNU libc is broken */
	struct e *last = STAILQ_LAST(&head, e, next);

	return (0);
}


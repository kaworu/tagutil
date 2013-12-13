#ifndef T_BACKEND_H
#define T_BACKEND_H
/*
 * t_backend.h
 *
 * backends functions for tagutil
 */
#include "t_config.h"
#include "t_tune.h"


struct t_backend {
	const char	*libid;
	const char	*desc;

	/*
	 * tune internal data (opaque) initialization.
	 *
	 * This routine can be used to detect if a backend can handle a
	 * particular file.
	 *
	 * @return
	 *   0 on success and tune is initialized, -1 otherwise.
	 */
	void *	(*init)(const char *path);

	/*
	 * Read all the tags from the storage.
	 *
	 * @return
	 *   a complete and ordered t_taglist or NULL on error.
	 */
	struct t_taglist *	(*read)(void *opaque);

	/*
	 * write the file.
	 *
	 * @return
	 *   return -1 on error, 0 on success.
	 */
	int	(*write)(void *opaque, const struct t_taglist *tlist);

	/*
	 * free internal data.
	 */
	void	(*clear)(void *opaque);

	TAILQ_ENTRY(t_backend)	entries;
};
TAILQ_HEAD(t_backendQ, t_backend);

/*
 * initialize backends if needed and return the list.
 */
const struct t_backendQ	*t_all_backends(void);

#endif /* ndef T_BACKEND_H */

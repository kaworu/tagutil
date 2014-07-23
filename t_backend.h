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
	 *   A pointer to the opaque data on success, NULL otherwise.
	 */
	void *	(*init)(const char *path);

	/*
	 * Read all the tags from the storage.
	 *
	 * @param opaque
	 *   an opaque pointer that has been provided by the init member
	 *   function.
	 *
	 * @return
	 *   a complete and ordered t_taglist or NULL on error.
	 */
	struct t_taglist *	(*read)(void *opaque);

	/*
	 * write the given taglist to the file.
	 *
	 * @param opaque
	 *   an opaque pointer that has been provided by the init member
	 *   function.
	 *
	 * @param tlist
	 *   the tag list to set for this tune.
	 *
	 * @return
	 *   return -1 on error, 0 on success.
	 */
	int	(*write)(void *opaque, const struct t_taglist *tlist);

	/*
	 * free internal data.
	 *
	 * After this function return, the opaque pointer must not be used
	 * anymore.
	 *
	 * @param opaque
	 *   an opaque pointer that has been provided by the init member
	 *   function.
	 */
	void	(*clear)(void *opaque);

	/* used for the backend queue */
	TAILQ_ENTRY(t_backend)	entries;
};
TAILQ_HEAD(t_backendQ, t_backend);

/*
 * initialize backends if needed and return the list.
 */
const struct t_backendQ	*t_all_backends(void);

#endif /* ndef T_BACKEND_H */

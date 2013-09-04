#ifndef T_BACKEND_H
#define T_BACKEND_H
/*
 * t_backend.h
 *
 * backends functions for tagutil
 */
#include "t_config.h"
#include "t_file.h"
#include "t_tune.h"


struct t_backend {
	const char	*libid;
	const char	*desc;

	struct t_file * (*ctor)(const char *path) t__deprecated;

	/*
	 * tune initialization.
	 *
	 * This routine can be used to detect if a backend can handle a
	 * particular file.
	 *
	 * @return
	 *   0 on success and tune is initialized, -1 otherwise.
	 */
	int  (*init)(struct t_tune *tune);

	/*
	 * Read all the tags from the storage.
	 *
	 * @return
	 *   a complete and ordered t_taglist or NULL on error.
	 */
	struct t_taglist * (*read)(struct t_tune *tune);

	/*
	 * write the file.
	 *
	 * @return
	 *   return -1 on error, 0 on success.
	 */
	int (*write)(struct t_tune *tune, const struct t_taglist *tags);

	/*
	 * free internal data.
	 */
	void (*clear)(struct t_tune *tune);

	TAILQ_ENTRY(t_backend)	entries;
};
TAILQ_HEAD(t_backendQ, t_backend);

/*
 * initialize backends if needed and return the list.
 */
const struct t_backendQ	*t_all_backends(void);

#endif /* not T_BACKEND_H */

#ifndef T_TUNE_H
#define T_TUNE_H
/*
 * t_tune.h
 *
 * A tune represent a music file with tags (or comments) attributes.
 */
#include <stdlib.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_error.h"
#include "t_tag.h"



/* abstract music file */
struct t_tune {
	char	*path;    /* the file's path */
	int	dirty;   /* 0 if clean (tags have not changed), >0 otherwise. */
	void	*opaque; /* pointer used by the backend's read and write routines */
	const struct t_backend	*backend; /* backend used to handle this file. */
	struct t_taglist	*tlist; /* used internal by t_tune routines. use t_tune_tags() instead */
};

/*
 * allocate memory for a new t_tune.
 *
 * @return
 *   a pointer to a fresh t_tune on success, NULL on error (malloc(3) failed).
 */
struct t_tune	*t_tune_new(const char *path);

/*
 * initialize internal data, find a backend able to handle the tune.
 *
 * @return
 *   0 on success and the t_tune is ready to be passed to t_tune_tags(),
 *   t_tune_set_tags() and t_tune_save() routines, -1 on error.
 */
int t_tune_init(struct t_tune *tune, const char *path);

/*
 * get all the tags of a tune.
 *
 * @return
 *   A complete and ordered t_taglist on success, NULL on error.
 */
const struct t_taglist	*t_tune_tags(struct t_tune *tune);

/*
 * set the tags for a tune.
 *
 * @return
 *   0 on success, -1 on error.
 */
int	t_tune_set_tags(struct t_tune *tune, const struct t_taglist *tlist);

/*
 * Save (write) the file to the storage with its new tags.
 *
 * @return
 *   0 on success, -1 on error.
 */
int	t_tune_save(struct t_tune *tune);

/*
 * free all the memory used internally by the t_tune.
 */
void	t_tune_clear(struct t_tune *tune);

/*
 * clear the t_tune and pass it to free(3). The pointer should not be used
 * afterward.
 */
void	t_tune_delete(struct t_tune *tune);

#endif /* not T_TUNE_H */

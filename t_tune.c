/*
 * t_tune.c
 *
 * A tune represent a music file with tags (or comments) attributes.
 */
#include "t_config.h"
#include "t_backend.h"
#include "t_tag.h"
#include "t_tune.h"


/* t_tune definition */
struct t_tune {
	char	*path;    /* the file's path */
	int	dirty;   /* 0 if clean (tags have not changed), >0 otherwise. */
	void	*opaque; /* pointer used by the backend's read and write routines */
	const struct t_backend	*backend; /* backend used to handle this file. */
	struct t_taglist	*tlist; /* used internal by t_tune routines. use t_tune_tags() instead */
};


/*
 * initialize internal data, find a backend able to handle the tune.
 *
 * @return
 *   0 on success and the t_tune is ready to be passed to t_tune_tags(),
 *   t_tune_set_tags() and t_tune_save() routines, -1 on error (ENOMEM) or if no
 *   backend was found.
 */
static int	t_tune_init(struct t_tune *tune, const char *path);

/*
 * free all the memory used internally by the t_tune.
 */
static void	t_tune_clear(struct t_tune *tune);


struct t_tune *
t_tune_new(const char *path)
{
	struct t_tune *tune;

	assert_not_null(path);

	tune = malloc(sizeof(struct t_tune));
	if (tune != NULL) {
		if (t_tune_init(tune, path) == -1) {
			free(tune);
			tune = NULL;
		}
	}

	return (tune);
}


static int
t_tune_init(struct t_tune *tune, const char *path)
{
	const struct t_backend  *b;
	const struct t_backendQ *bQ;

	assert_not_null(tune);
	assert_not_null(path);

	bzero(tune, sizeof(struct t_tune));
	tune->path = strdup(path);
	if (tune->path == NULL)
		return (-1);

	bQ = t_all_backends();
	TAILQ_FOREACH(b, bQ, entries) {
		if (b->init != NULL) {
			void *o = b->init(tune->path);
			if (o != NULL) {
				tune->backend = b;
				tune->opaque  = o;
				return (0);
			}
		}
	}

	/* no backend found */
	free(tune->path);
	tune->path = NULL;
	return (-1);
}


struct t_taglist *
t_tune_tags(struct t_tune *tune)
{

	assert_not_null(tune);

	if (tune->tlist == NULL)
		tune->tlist = tune->backend->read(tune->opaque);

	return (t_taglist_clone(tune->tlist));
}


const char *
t_tune_path(struct t_tune *tune)
{
	assert_not_null(tune);

	return (tune->path);
}


const struct t_backend *
t_tune_backend(struct t_tune *tune)
{
	assert_not_null(tune);

	return (tune->backend);
}


int
t_tune_set_tags(struct t_tune *tune, const struct t_taglist *tlist)
{
	assert_not_null(tune);
	assert_not_null(tlist);

	if (tune->tlist != tlist) {
		t_taglist_delete(tune->tlist);
		tune->tlist = t_taglist_clone(tlist);
		if (tune->tlist == NULL)
			return (-1);
		tune->dirty++;
	}

	return (0);
}


int
t_tune_save(struct t_tune *tune)
{

	assert_not_null(tune);

	if (tune->dirty) {
		if (tune->backend->write(tune->opaque, tune->tlist) == 0) {
			/* success */
			tune->dirty = 0;
		}
	}

	return (tune->dirty ? -1 : 0);
}


static void
t_tune_clear(struct t_tune *tune)
{

	assert_not_null(tune);

	/* tune is either initialized with both path and backend set, or it's
	 uninitialized */
	if (tune->backend != NULL)
		tune->backend->clear(tune->opaque);
	t_taglist_delete(tune->tlist);
	free(tune->path);
	bzero(tune, sizeof(struct t_tune));
}


void
t_tune_delete(struct t_tune *tune)
{

	if (tune != NULL)
		t_tune_clear(tune);
	free(tune);
}

/* this is needed because we don't expose t_tune_init nor t_tune_clear. The
   renamer does something pretty unusual so it's implemented that way for the
   time being. */
int
t__tune_reload__(struct t_tune *tune, const char *path)
{
	assert_not_null(tune);

	t_tune_clear(tune);
	return (t_tune_init(tune, path));
}

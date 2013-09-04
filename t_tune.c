/*
 * t_tune.c
 *
 * A tune represent a music file with tags (or comments) attributes.
 */
#include "t_config.h"
#include "t_backend.h"
#include "t_tag.h"
#include "t_tune.h"


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

int
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
		if (b->init(tune) == 0)
			return (0);
	}

	free(tune->path);
	tune->path = NULL;
	return (-1);
}

struct t_taglist *
t_tune_tags(struct t_tune *tune)
{

	assert_not_null(tune);
	assert_not_null(tune->backend);

	if (tune->tlist == NULL)
		tune->tlist = tune->backend->read(tune);

	return (t_taglist_clone(tune->tlist));
}

int
t_tune_set_tags(struct t_tune *tune, const struct t_taglist *tlist)
{
	assert_not_null(tune);
	assert_not_null(tune->backend); /* this is not really needed, but we ensure a safe initialization */
	assert_not_null(tlist);

	t_taglist_delete(tune->tlist);
	tune->tlist = t_taglist_clone(tlist);
	tune->dirty++;

	return (tune->tlist == NULL ? -1 : 0);
}

int
t_tune_save(struct t_tune *tune)
{
	int ret = 0;

	assert_not_null(tune);
	assert_not_null(tune->backend);

	if (tune->dirty) {
		ret = tune->backend->write(tune, tune->tlist);
		if (ret == 0) /* success */
			tune->dirty = 0;
	}

	return (ret);
}

void
t_tune_clear(struct t_tune *tune)
{

	assert_not_null(tune);

	/* tune is either initialized with both path and backend set, or it's
	 uninitialized */
	if (tune->backend != NULL)
		tune->backend->clear(tune);
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

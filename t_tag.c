/*
 * t_tag.c
 *
 * tagutil's tag routines.
 */
#include <string.h>

#include "t_config.h"
#include "t_error.h"
#include "t_tag.h"


struct t_taglist *
t_taglist_new(void)
{
	struct t_taglist *ret;

	ret = malloc(sizeof(struct t_taglist) + sizeof(struct t_tagQ));
	if (ret == NULL)
		return (NULL);

	ret->tags = (struct t_tagQ *)(ret + 1);
	ret->count = 0;
	t_error_init(ret);
	TAILQ_INIT(ret->tags);

	return (ret);
}

struct t_taglist *
t_taglist_clone(const struct t_taglist *tlist)
{
	struct t_taglist *clone;
	struct t_tag *t;

	if (tlist == NULL)
		return (NULL);

	clone = t_taglist_new();
	if (clone == NULL)
		return (NULL);
	TAILQ_FOREACH(t, tlist->tags, entries) {
		if (t_taglist_insert(clone, t->key, t->val) != 0) {
			t_taglist_delete(clone);
			return (NULL);
		}
	}

	return (clone);
}

int
t_taglist_insert(struct t_taglist *T, const char *key, const char *val)
{
	size_t klen, vlen;
	struct t_tag *t;
	char *s;

	assert_not_null(T);
	assert_not_null(key);
	assert_not_null(val);

	klen = strlen(key);
	vlen = strlen(val);
	t    = malloc(sizeof(struct t_tag) + klen + 1 + vlen + 1);
	if (t == NULL)
		return (-1);

	t->klen = klen;
	t->vlen = vlen;
	t->key = s = (char *)(t + 1);
	assert(strlcpy(s, key, t->klen + 1) == t->klen);
	t_strtolower(s);
	t->val = s = (char *)(s + t->klen + 1);
	assert(strlcpy(s, val, t->vlen + 1) == t->vlen);

	T->count++;
	TAILQ_INSERT_TAIL(T->tags, t, entries);

	return (0);
}


char *
t_taglist_join(const struct t_taglist *T, const char *glue)
{
	struct sbuf *sb;
	struct t_tag *t, *last;
	char *ret;

	assert_not_null(T);
	assert_not_null(glue);

	if (T->count == 0)
		return (calloc(1, sizeof(char)));
	sb = sbuf_new_auto();
	if (sb == NULL)
		return (NULL);

	last = TAILQ_LAST(T->tags, t_tagQ);
	TAILQ_FOREACH(t, T->tags, entries) {
		(void)sbuf_cat(sb, t->val);
		if (t != last)
			(void)sbuf_cat(sb, glue);
	}

	if (sbuf_finish(sb) == -1) {
		sbuf_delete(sb);
		return (NULL);
	}
	ret = strdup(sbuf_data(sb));
	sbuf_delete(sb);

	return (ret);
}


struct t_tag *
t_taglist_tag_at(const struct t_taglist *T, unsigned int index)
{
	struct t_tag *t;

	assert_not_null(T);

	t = TAILQ_FIRST(T->tags);
	while (t != NULL && index-- > 0)
		t = TAILQ_NEXT(t, entries);

	return (t);
}


void
t_taglist_delete(struct t_taglist *T)
{
	struct t_tag *t1, *t2;

	if (T == NULL)
		return;

	t1 = TAILQ_FIRST(T->tags);
	while (t1 != NULL) {
		t2 = TAILQ_NEXT(t1, entries);
		free(t1);
		t1 = t2;
	}
	t_error_clear(T);
	free(T);
}

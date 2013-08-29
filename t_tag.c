/*
 * t_tag.c
 *
 * tagutil's tag structures/functions.
 */
#include <string.h>

#include "t_config.h"
#include "t_toolkit.h"
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


int
t_taglist_insert(struct t_taglist *T,
        const char *key, const char *val)
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
	t->klen   = klen;
	t->vlen = vlen;
	t->key = s = (char *)(t + 1);
	assert(strlcpy(s, key, t->klen + 1) == t->klen);
	t_strtolower(s);
	t->val = s = (char *)(s + t->klen + 1);
	assert(strlcpy(s, val, t->vlen + 1) == t->vlen);

	TAILQ_INSERT_TAIL(T->tags, t, entries);
	T->count++;
	return (0);
}


char *
t_taglist_join(struct t_taglist *T, const char *j)
{
	struct sbuf *sb;
	struct t_tag *t, *last;
	char *retval;

	assert_not_null(T);

	if (T->count == 0)
		return (xcalloc(1, sizeof(char)));

	sb = sbuf_new_auto();
	if (sb == NULL)
		err(errno, "sbuf_new");
	last = TAILQ_LAST(T->tags, t_tagQ);

	TAILQ_FOREACH(t, T->tags, entries) {
		(void)sbuf_cat(sb, t->val);
		if (t != last)
			(void)sbuf_cat(sb, j);
	}

	if (sbuf_finish(sb) == -1)
		err(errno, "sbuf_finish");
	retval = xstrdup(sbuf_data(sb));
	sbuf_delete(sb);
	return (retval);
}


struct t_tag *
t_taglist_tag_at(const struct t_taglist *T, unsigned int idx)
{
    struct t_tag *t;

    assert_not_null(T);

    t = TAILQ_FIRST(T->tags);
    while (t != NULL && idx-- > 0)
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
		freex(t1);
		t1 = t2;
	}
	t_error_clear(T);
	freex(T);
}


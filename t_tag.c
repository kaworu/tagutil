/*
 * t_tag.h
 *
 * a tag (or "comment").
 */
#include <strings.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_tag.h"


struct t_tag *
t_tag_new(const char *key, const char *val)
{
	struct t_tag *t;
	char *s;
	size_t klen, vlen;

	assert_not_null(key);
	assert_not_null(val);
	klen = strlen(key);
	vlen = strlen(val);

	t = malloc(sizeof(struct t_tag) + klen + 1 + vlen + 1);
	if (t == NULL)
		return (NULL);

	t->klen = klen;
	t->vlen = vlen;
	t->key = s = (char *)(t + 1);
	(void)memcpy(s, key, t->klen + 1);
	t_strtolower(s);
	t->val = s = (char *)(s + t->klen + 1);
	(void)memcpy(s, val, t->vlen + 1);

	return (t);
}


int
t_tag_keycmp(const char *x, const char *y)
{

	return (strcasecmp(x, y));
}


void
t_tag_delete(struct t_tag *victim)
{

	free(victim);
}

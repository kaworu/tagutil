#ifndef T_TAG_H
#define T_TAG_H
/*
 * t_tag.h
 *
 * a tag (or "comment").
 */
#include "t_config.h"

/*
 * key / value pair, element of a list (implemented as TAILQ).
 */
struct t_tag {
	size_t		klen;
	size_t		vlen;
	const char	*key;
	const char	*val;
	TAILQ_ENTRY(t_tag)	entries;
};
TAILQ_HEAD(t_tagQ, t_tag);


/*
 * create a new tag.
 *
 * The returned t_tag should be passed to free(3) after use.
 *
 * @return
 *   a new t_tag or NULL or error (malloc(3) failed).
 */
struct t_tag *	t_tag_new(const char *key, const char *val);

/*
 * compare two tag keys.
 *
 * This routine should be used when key comparison is needed.
 *
 * @param k1
 *   The first key to compare, cannot be NULL.
 *
 * @param k2
 *   The second key to compare, cannot be NULL.
 *
 * @return
 *   an integer greater than, equal to, or less than 0, according as the string
 *   k1 is greater than, equal to, or less than the string k2.
 */
int	t_tag_keycmp(const char *k1, const char *k2);

/*
 * free a t_tag and its associated data.
 *
 * @param victim
 *   The t_tag to free.
 */
void t_tag_delete(struct t_tag *victim);

#endif /* ndef T_TAG_H */

#ifndef T_TAG_H
#define T_TAG_H
/*
 * t_tag.h
 *
 * tagutil's tag structures/functions.
 */
#include <stdbool.h>

#include "t_config.h"
#include "t_error.h"


/*
 * a tag.
 *
 * a key / value pair, element of a list (implemented as TAILQ).
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
 * a list of tags.
 *
 * abstract structure for a music file's tags. t_error macros can (and should)
 * be used on this structure.
 */
struct t_taglist {
	size_t		count;
	struct t_tagQ	*tags;

	T_ERROR_MSG_MEMBER;
};


/*
 * create a new t_taglist.
 *
 * The returned t_taglist is empty and should be passed to t_taglist_delete()
 * after use.
 *
 * @return
 *   a t_taglist pointer on success, NULL and set errno on error (malloc(3)
 *   failed).
 */
struct t_taglist	*t_taglist_new(void);

/*
 * insert a tag in a tag list.
 *
 * @param key
 *   The tag's key
 *
 * @param val
 *   The tag's value
 *
 * @return
 *   0 on success, -1 and set errno on error (malloc(3) failed).
 */
int	t_taglist_insert(struct t_taglist *T, const char *key,
	    const char *val);

/*
 * return the tag at index idx, or NULL.
 */
struct t_tag	*t_taglist_tag_at(const struct t_taglist *T, unsigned int idx);

/*
 * join all the tag in T with j.
 * if j is NULL, it has the same effect as "".
 *
 * returned value has to be free()d.
 */
char * t_taglist_join(struct t_taglist *T, const char *j);

/*
 * free the t_taglist struct and all the t_tag (0 indexed).
 */
void t_taglist_delete(struct t_taglist *T);

#endif /* not T_TAG_H */

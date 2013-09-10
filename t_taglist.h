#ifndef T_TAGLIST_H
#define T_TAGLIST_H
/*
 * t_taglist.h
 *
 * tagutil's tag lists.
 */
#include "t_config.h"
#include "t_tag.h"
#include "t_error.h"


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
 * create a deep copy of a taglist.
 *
 * The returned should be passed to t_taglist_delete() after use.
 *
 * @return
 *   a t_taglist pointer on success, NULL and set errno on error (malloc(3)
 *   failed).
 */
struct t_taglist	*t_taglist_clone(const struct t_taglist *tags);

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
int	t_taglist_insert(struct t_taglist *tlist, const char *key,
	    const char *val);

/*
 * Find all tags matching key in tlist.
 *
 * @param tlist
 *   The t_taglist to browse, cannot be NULL.
 *
 * @param key
 *   The key to match, cannot be NULL.
 *
 * @return
 *   a t_taglist with all tags matching key found in the given tlist. On error
 *   NULL is returned (malloc(3) failed). The returned value should be passed to
 *   t_taglist_delete() after use.
 */
struct t_taglist	*t_taglist_find_all(const struct t_taglist *tlist,
			    const char *key);

/*
 * return the tag at given index, or NULL.
 */
struct t_tag	*t_taglist_tag_at(const struct t_taglist *tlist, unsigned int index);

/*
 * join all the tag values in tlist with the given glue.
 *
 * @param tlist
 *   The taglist containing all the tag to join.
 *
 * @param glue
 *   inserted between each tag value.
 *
 * @return
 *   NULL on error and a (char *) that should be passed to free(3) after use.
 */
char	*t_taglist_join(const struct t_taglist *tlist, const char *glue);

/*
 * free the t_taglist and all its tags.
 *
 * A t_taglist should not be used after a call to t_taglist_delete().
 *
 * @param tlist
 *   The taglist to free
 */
void	t_taglist_delete(struct t_taglist *tlist);

#endif /* not T_TAGLIST_H */

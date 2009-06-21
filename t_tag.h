#ifndef T_TAG_H
#define T_TAG_H
/*
 * t_tag.h
 *
 * tagutil's tag structures/functions.
 */
#include <sys/queue.h>

#include <stdbool.h>

#include "t_config.h"
#include "t_error.h"


/* tag key/values */
struct t_tag {
    size_t keylen, valuelen;
    const char *key, *value;
    STAILQ_ENTRY(t_tag) _t__next;
};
STAILQ_HEAD(t_tagQ, t_tag);

#define t_tagQ_init(x)              STAILQ_INIT(x)
#define t_tagQ_insert(head, elt)    STAILQ_INSERT_TAIL((head), (elt), _t__next)
#define t_tagQ_empty(head)          STAILQ_EMPTY(head)
#define t_tagQ_first(head)          STAILQ_FIRST(head)
#define t_tagQ_last(head)           STAILQ_LAST((head), t_tag, _t__next)
#define t_tagQ_next(elt)            STAILQ_NEXT(elt, _t__next)
#define t_tagQ_foreach(elt, head)   STAILQ_FOREACH((elt), (head), _t__next)

/*
 * a tag list, abstract structure for a music file's tags.
 * t_error macros can (and should) be used on this structure.
 */
struct t_taglist {
    size_t count;
    struct t_tagQ *tags;

    ERROR_MSG_MEMBER;
    unsigned int childcount;
    struct t_taglist *parent;
};


/*
 * create a new t_taglist.
 * returned value has to be free()d (see t_taglist_destroy()).
 */
struct t_taglist * t_taglist_new(void);

/*
 * insert a new key/value in given list.
 */
_t__nonnull(1) _t__nonnull(2) _t__nonnull(3)
void t_taglist_insert(struct t_taglist *restrict T,
        const char *restrict key, const char *restrict value);

#define T_TAG_FIRST true
#define T_TAG_ALL   false
/*
 * find all t_tag in list matching given key.
 *
 * if onlyfirst is true, the returned list has only one element, the first
 * t_tag matching key.
 *
 * Be aware that the returned value is not a complete copy, it becomes a child
 * of T and *must* be freed via t_taglist_destroy before its parent.
 *
 * return a new list if found at least one matching tag, or NULL otherwise.
 */
_t__nonnull(1) _t__nonnull(2)
struct t_taglist * t_taglist_filter(const struct t_taglist *restrict T,
        const char *restrict key, bool onlyfirst);

/*
 * count the number of tag matching key.
 *
 * if onlyone is true, the function return as soon as it found a matching key
 * (then, returning one), or return 0.
 */
_t__nonnull(1) _t__nonnull(2)
unsigned int t_taglist_filter_count(const struct t_taglist *restrict T,
        const char *restrict key, bool onlyfirst);
/*
 * return the tag at index idx, or NULL if idx > T->count.
 */
_t__nonnull(1)
struct t_tag * t_taglist_tag_at(struct t_taglist *restrict T,
        unsigned int idx);

/*
 * join all the tag in T with j.
 * if j is NULL, it has the same effect as "".
 *
 * returned value has to be free()d.
 */
_t__nonnull(1)
char * t_taglist_join(struct t_taglist *restrict T, const char *restrict j);

/*
 * free the t_taglist struct and all the t_tag (0 indexed).
 */
_t__nonnull(1)
void t_taglist_destroy(struct t_taglist *T);

#endif /* not T_TAG_H */

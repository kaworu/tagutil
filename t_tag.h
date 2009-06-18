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
#include "t_toolkit.h"
#include "t_error.h"


/* tag value */
struct t_tagv {
    size_t vlen;
    const char *value;
    TAILQ_ENTRY(t_tagv) next;
};
TAILQ_HEAD(t_tagv_q, t_tagv);

/* tag key/values */
struct t_tag {
    size_t keylen, vcount;
    const char *key;
    struct t_tagv_q *values;
    TAILQ_ENTRY(t_tag) next;
};
TAILQ_HEAD(t_tag_q, t_tag);


/*
 * return the t_tagv value at index idx.
 * if idx >= t->vcount (i.e. idx is a bad index) NULL is returned.
 */
_t__unused _t__nonnull(1)
static inline struct t_tagv * t_tag_value_by_idx(struct t_tag *restrict t,
        size_t idx);


/*
 * a tag list, abstract structure for a music file's tags.
 * It's designed to handle several key with each several possibles values.
 *
 * errors macros defined in t_toolkit.h can (and should) be used on this
 * structure.
 */
struct t_taglist {
    size_t tcount;
    struct t_tag_q *tags;

    ERROR_MSG_MEMBER;
};


/*
 * create a new t_taglist.
 * returned value has to be free()d (see t_taglist_destroy()).
 */
struct t_taglist * t_taglist_new(void);

/*
 * insert a new key/value in T.
 * value can be NULL (for example, if you need to use the t_taglist for a
 * clear() call).
 */
_t__nonnull(1) _t__nonnull(2)
void t_taglist_insert(struct t_taglist *restrict T,
        const char *restrict key, const char *restrict value);

/*
 * find a t_tag in T matching given key.
 *
 * return the matching t_tag if found, or NULL otherwhise.
 */
_t__nonnull(1) _t__nonnull(2)
struct t_tag * t_taglist_search(const struct t_taglist *restrict T,
        const char *restrict key);


/*
 * free the t_taglist structure and all the t_tag/t_tagv.
 */
_t__nonnull(1)
void t_taglist_destroy(const struct t_taglist *T);

/********************************************************************************/

static inline struct t_tagv *
t_tag_value_by_idx(struct t_tag *restrict t, size_t idx)
{
    struct t_tagv *ret = NULL;

    assert_not_null(t);

    if (idx < t->vcount) {
        ret = TAILQ_FIRST(t->values);
        while (idx--)
            ret = TAILQ_NEXT(ret, next);
    }

    return (ret);
}
#endif /* not T_TAG_H */

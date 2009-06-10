#ifndef T_TAG_H
#define T_TAG_H
/*
 * t_tag.h
 *
 * tagutil's tag structures/functions.
 */
#include "t_config.h"

#include <sys/queue.h>

#include <stdbool.h>

#include "t_toolkit.h"


/* tag value */
struct ttagv {
    ssize_t vallen;
    const char *val;
    TAILQ_ENTRY(ttagv) next;
};
TAILQ_HEAD(ttagv_q, ttagv);

/* tag key/values */
struct ttag {
    size_t keylen, vcount /* == 0 if deleted is needed */; 
    const char *key;
    struct ttagv_q *values;
    TAILQ_ENTRY(ttag) next;
};
TAILQ_HEAD(ttag_q, ttag);

struct tag_list {
    bool frozen;
    size_t tcount;
    struct ttag_q *tags;
};


#define ttag_delete(t) ((t)->vcount == 0)
/*
 * TODO
 */
struct tag_list * new_tag_list(void);


/*
 * TODO
 */
_t__nonnull(1) _t__nonnull(2)
bool tag_list_insert(struct tag_list *restrict T,
        const char *restrict key, const char *restrict val,
        char **emsg);


/*
 * TODO
 */
_t__nonnull(1) _t__nonnull(2)
struct ttag * tag_list_search(struct tag_list *restrict T,
        const char *restrict key);


/*
 * TODO
 */
_t__nonnull(1)
void destroy_tag_list(struct tag_list *restrict T);

#endif /* not T_TAG_H */

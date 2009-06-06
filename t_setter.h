#ifndef T_SETTER_H
#define T_SETTER_H
/*
 * t_setter.h
 *
 * structure and utils helping tagutil to set tags.
 */
#include "t_config.h"

#include <sys/queue.h>
#include <stdbool.h>
#include <string.h>

#include "t_toolkit.h"


struct setter_item {
    const char *key, *value;
    STAILQ_ENTRY(setter_item) next;
};

STAILQ_HEAD(setter_q, setter_item);

/*
 * create a new setter queue.
 * returned value has to be free()d (use destroy_setter).
 */
static inline struct setter_q * new_setter(void);

/*
 * add a key/value pair to to the queue.
 * return true if key/value was appened to the queue, false otherwise.
 */
static inline void setter_add(struct setter_q *restrict Q,
        const char *restrict key, const char *restrict value);

/*
 * free the queue and all its elements.
 */
static inline void destroy_setter(struct setter_q *restrict Q);


static inline struct setter_q *
new_setter(void)
{
    struct setter_q *Q;

    Q = xmalloc(sizeof(struct setter_q));
    STAILQ_INIT(Q);

    return (Q);
}


static inline void
setter_add(struct setter_q *restrict Q,
        const char *restrict key, const char *restrict value)
{
    struct setter_item *item;
    size_t size, keylen, vallen;
    char *s;

    assert_not_null(Q);
    assert_not_null(key);

    keylen = strlen(key);
    size = keylen + 1;
    if (value) {
        vallen = strlen(value);
        size += vallen + 1;
    }

    item = xmalloc(sizeof(struct setter_item) + size);
    s = (char *)(item + 1);
    (void)strlcpy(s, key, size);
    item->key = s;

    if (value) {
        s += keylen + 1;
        (void)strlcpy(s, value, size - (keylen + 1));
        item->value = s;
    }
    else
        item->value = NULL;

    STAILQ_INSERT_TAIL(Q, item, next);
}


static inline void
destroy_setter(struct setter_q *restrict Q)
{
    struct setter_item *item;

    assert_not_null(Q);

    while (!STAILQ_EMPTY(Q)) {
        item = STAILQ_FIRST(Q);
        STAILQ_REMOVE_HEAD(Q, next);
        xfree(item);
    }
    xfree(Q);
}
#endif /* not T_SETTER_H */

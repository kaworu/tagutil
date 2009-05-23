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
 * add a key/value pair to to the queue. the keyval arg should be like
 * "key=value" where value can be empty.
 * return true if keyval is ok and appened to the queue, false otherwise.
 */
static inline bool setter_add(struct setter_q *restrict Q, const char *keyval);

/*
 * add a key/value pair to to the queue.
 * return true if key/value was appened to the queue, false otherwise.
 */
static inline bool setter_add2(struct setter_q *restrict Q, const char *key,
        const char *value);

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


static inline bool
setter_add(struct setter_q *restrict Q, const char *keyval)
{
    struct setter_item *it;
    size_t size;
    char *sep, *s;

    assert_not_null(Q);
    assert_not_null(keyval);

    size = (strlen(keyval) + 1) * sizeof(char);
    it = xmalloc(sizeof(struct setter_item) + size);
    s = (char *)(it + 1);
    (void)strlcpy(s, keyval, size);

    sep = strchr(s, '=');
    if (sep == NULL) {
        xfree(it);
        return (false);
    }
    *sep = '\0';
    it->key = s;
    it->value = sep + 1;

    STAILQ_INSERT_TAIL(Q, it, next);

    return (true);
}


static inline bool
setter_add2(struct setter_q *restrict Q, const char *key, const char *value)
{
    struct setter_item *it;
    size_t size, keylen, vallen;
    char *s;

    assert_not_null(Q);
    assert_not_null(key);
    assert_not_null(value);

    keylen = strlen(key);
    vallen = strlen(value);
    size = (keylen + 1 + vallen + 1) * sizeof(char);
    it = xmalloc(sizeof(struct setter_item) + size);
    s = (char *)(it + 1);
    (void)strlcpy(s, key, size);
    it->key = s;
    s += keylen + 1;
    (void)strlcpy(s, value, size - (keylen + 1));
    it->value = s;

    STAILQ_INSERT_TAIL(Q, it, next);

    return (true);
}


static inline void
destroy_setter(struct setter_q *restrict Q)
{
    struct setter_item *it;

    assert_not_null(Q);

    while (!STAILQ_EMPTY(Q)) {
        it = STAILQ_FIRST(Q);
        STAILQ_REMOVE_HEAD(Q, next);
        xfree(it);
    }
    xfree(Q);
}
#endif /* not T_SETTER_H */

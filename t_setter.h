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


#define SETTER_SEP ':'


struct setter_item {
    const char *key, *value;
    STAILQ_ENTRY(setter_item) next;
};

STAILQ_HEAD(setter_q, setter_item);


static inline struct setter_q *
setter_init(void)
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
    size_t siz;
    char *sep, *s;

    assert_not_null(Q);
    assert_not_null(keyval);

    siz = (strlen(keyval) + 1) * sizeof(char);
    it = xmalloc(sizeof(struct setter_item) + siz);
    s = (char *)(it + 1);
    strlcpy(s, keyval, siz);

    sep = strchr(s, SETTER_SEP);
    if (sep == NULL) {
        free(it);
        return (false);
    }
    *sep = '\0';
    it->key = s;
    it->value = sep + 1;

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

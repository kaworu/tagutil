/*
 * t_tag.c
 *
 * tagutil's tag structures/functions.
 */
#include "t_config.h"

#include "t_tag.h"
#include "t_toolkit.h"


struct tag_list *
new_tag_list(void)
{
    struct tag_list *ret;

    ret = xmalloc(sizeof(struct tag_list) + sizeof(struct ttag_q));
    ret->tags = (struct ttag_q *)(ret + 1);
    ret->tcount = 0;
    last_error_msg(ret) = NULL;
    TAILQ_INIT(ret->tags);

    return (ret);
}


void
tag_list_insert(struct tag_list *restrict T,
        const char *restrict key, const char *restrict value)
{
    size_t len;
    ssize_t vlen;
    char *s;
    struct ttag  *t, *kinq;
    struct ttagv *v;

    assert_not_null(key);
    assert_not_null(T);

    /* create v if needed */
    if (value) {
        vlen = strlen(value);
        v = xmalloc(sizeof(struct ttagv) + vlen + 1);
        v->vallen = vlen;
        s = (char *)(v + 1);
        (void)strlcpy(s, value, vlen + 1);
        v->value = s;
    }
    else
        v = NULL;

    /* look if a ttag matching key already exist */
    len = strlen(key);
    kinq = NULL;
    TAILQ_FOREACH_REVERSE(t, T->tags, ttag_q, next) {
        if (t->keylen == len && strcasecmp(key, t->key) == 0) {
            kinq = t;
            break;
        }
    }

    if (kinq == NULL) {
    /* doesn't exist, create a new ttag (delete) */
        kinq = xmalloc(sizeof(struct ttag) + sizeof(struct ttagv_q) + len + 1);
        TAILQ_INSERT_TAIL(T->tags, kinq, next);
        T->tcount++;

        kinq->values = (struct ttagv_q *)(kinq + 1);
        TAILQ_INIT(kinq->values);
        kinq->vcount = 0;

        kinq->keylen = len;
        s = (char *)(kinq->values + 1);
        (void)strlcpy(s, key, len + 1);
        kinq->key = s;

    }

    if (v) {
        TAILQ_INSERT_TAIL(kinq->values, v, next);
        kinq->vcount++;
    }
}


struct ttag *
tag_list_search(const struct tag_list *restrict T, const char *restrict key)
{
    size_t len;
    struct ttag  *t, *target;

    assert_not_null(T);
    assert_not_null(key);

    len = strlen(key);
    target = NULL;
    TAILQ_FOREACH(t, T->tags, next) {
        if (len == t->keylen && strcasecmp(t->key, key) == 0) {
            target = t;
            break;
        }
    }

    return (target);
}


void
destroy_tag_list(const struct tag_list *Tconst)
{
    struct ttag  *t;
    struct ttagv *v;
    struct tag_list *T;

    T = (struct tag_list *)Tconst; /* break const */

    assert_not_null(T);

    while (!TAILQ_EMPTY(T->tags)) {
        t = TAILQ_FIRST(T->tags);
        TAILQ_REMOVE(T->tags, t, next);
        while (!TAILQ_EMPTY(t->values)) {
            v = TAILQ_FIRST(t->values);
            TAILQ_REMOVE(t->values, v, next);
            xfree(v);
        }
        xfree(t);
    }
    reset_error_msg(T);
    xfree(T);
}


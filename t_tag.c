/*
 * t_tag.c
 *
 * tagutil's tag structures/functions.
 */
#include <string.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_error.h"
#include "t_strbuffer.h"
#include "t_tag.h"


struct t_taglist *
t_taglist_new(void)
{
    struct t_taglist *ret;

    ret = xmalloc(sizeof(struct t_taglist) + sizeof(struct t_tagQ));
    ret->tags = (struct t_tagQ *)(ret + 1);
    ret->count = 0;
    ret->childcount = 0;
    ret->parent = NULL;
    t_error_init(ret);
    t_tagQ_init(ret->tags);

    return (ret);
}


void
t_taglist_insert(struct t_taglist *restrict T,
        const char *restrict key, const char *restrict value)
{
    size_t klen, vlen;
    struct t_tag *t;
    char *s;

    assert_not_null(T);
    assert_not_null(key);
    assert_not_null(value);

    klen = strlen(key);
    vlen = strlen(value);

    t = xmalloc(sizeof(struct t_tag) + klen + 1 + vlen + 1);
    t->keylen = klen;
    t->valuelen = vlen;
    t->key = s = (char *)(t + 1);
    assert(strlcpy(s, key, t->keylen + 1) < (t->keylen + 1));
    t_strtolower(s);
    t->value = s = (char *)(s + t->keylen + 1);
    assert(strlcpy(s, value, t->valuelen + 1) < (t->valuelen + 1));

    t_tagQ_insert(T->tags, t);
    T->count++;
}


struct t_taglist *
t_taglist_filter(const struct t_taglist *restrict T,
        const char *restrict key, bool onlyfirst)
{
    size_t len;
    struct t_taglist *ret;
    struct t_tag *t, *n;

    assert_not_null(T);
    assert_not_null(key);

    ret = NULL;
    len = strlen(key);
    t_tagQ_foreach(t, T->tags) {
        if (len == t->keylen && strcasecmp(t->key, key) == 0) {
            if (ret == NULL)
                ret = t_taglist_new();
            n = xmalloc(sizeof(struct t_tag));
            n->keylen   = t->keylen;
            n->key      = t->key;
            n->valuelen = t->valuelen;
            n->value    = t->value;
            t_tagQ_insert(ret->tags, n);
            ret->count++;
            if (onlyfirst)
                break;
        }
    }

    if (ret != NULL) {
    /* ret is now a child of T */
        /* break const, look how bad we are :) */
        ret->parent = (struct t_taglist *)T;
        ret->parent->childcount++;
        if (onlyfirst)
            assert(ret->count == 1);
    }

    return (ret);
}


unsigned int
t_taglist_filter_count(const struct t_taglist *restrict T,
        const char *restrict key, bool onlyfirst)
{
    size_t len;
    unsigned int ret;
    struct t_tag *t;

    assert_not_null(T);
    assert_not_null(key);

    ret = 0;
    len = strlen(key);
    t_tagQ_foreach(t, T->tags) {
        if (len == t->keylen && strcasecmp(t->key, key) == 0) {
            ret++;
            if (onlyfirst)
                break;
        }
    }

    return (ret);
}


char *
t_taglist_join(struct t_taglist *restrict T, const char *restrict j)
{
    size_t jlen;
    struct t_strbuffer *sb;
    struct t_tag *t, *last;

    assert_not_null(T);

    if (T->count == 0)
        return (xcalloc(1, sizeof(char)));

    jlen = strlen(j);
    sb = t_strbuffer_new();
    last = t_tagQ_last(T->tags);

    t_tagQ_foreach(t, T->tags) {
        t_strbuffer_add(sb, t->value, t->valuelen, T_STRBUFFER_NOFREE);
        if (t != last)
            t_strbuffer_add(sb, j, jlen, T_STRBUFFER_NOFREE);
    }

    return (t_strbuffer_get(sb));
}


struct t_tag *
t_taglist_tag_at(struct t_taglist *restrict T, unsigned int idx)
{
    struct t_tag *t;

    assert_not_null(T);

    t = t_tagQ_first(T->tags);
    while (t != NULL && idx > 0) {
        idx--;
        t = t_tagQ_next(t);
    }

    return (t);
}


void
t_taglist_destroy(struct t_taglist *restrict T)
{
    struct t_tag *t1, *t2;

    assert_not_null(T);
    assert(T->childcount == 0);

    if (T->parent != NULL) {
        assert(T->parent->childcount > 0);
        T->parent->childcount--;
    }

    t1 = t_tagQ_first(T->tags);
    while (t1 != NULL) {
        t2 = t_tagQ_next(t1);
        freex(t1);
        t1 = t2;
    }
    t_error_clear(T);
    freex(T);
}


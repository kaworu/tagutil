#ifndef T_STRBUFFER_H
#define T_STRBUFFER_H
/*
 * t_strbuffer.h
 *
 * String Buffer utils.
 *
 */
#include <stdbool.h>
#include <stdlib.h>
#include <string.h> /* memcpy(3) */

#include "t_config.h"
#include "t_toolkit.h"


#define T_STRBUFFER_INIT_BCOUNT 8

struct t_strbuffer {
    const char **buffers;
    int bcount, blast;
    size_t *blen, len;
    bool *bfree;
};


/*
 * create and init a t_strbuffer struct.
 */
_t__unused
static inline struct t_strbuffer * t_strbuffer_new(void);

/*
 * add the given s to sb.
 *
 * len should be number of bytes in s. s could be not NUL-terminated.
 *
 * if mine, the s pointer should be free()able and is owned by the t_strbuffer
 * after the call. Otherwhise the caller is responsible to avoid modification
 * to s before destroying the buffer.
 */
_t__unused _t__nonnull(1) _t__nonnull(2)
static inline void t_strbuffer_add(struct t_strbuffer *sb,
        const char *s, size_t len, bool mine);
#define T_STRBUFFER_FREE true
#define T_STRBUFFER_NOFREE false

/*
 * return the result of the buffer *and* destroy the buffer.
 *
 * returned value has to be free()d.
 */
_t__unused _t__nonnull(1)
static inline char * t_strbuffer_get(struct t_strbuffer *sb);

/*
 * free the given t_strbuffer.
 */
_t__unused _t__nonnull(1)
static inline void t_strbuffer_destroy(struct t_strbuffer *sb);


static inline struct t_strbuffer *
t_strbuffer_new(void)
{
    struct t_strbuffer *ret;

    ret = xmalloc(sizeof(struct t_strbuffer));

    ret->len     =  0;
    ret->bcount  =  T_STRBUFFER_INIT_BCOUNT;
    ret->blast   = -1;
    ret->buffers = xcalloc(ret->bcount, sizeof(char *));
    ret->blen    = xcalloc(ret->bcount, sizeof(size_t));
    ret->bfree   = xcalloc(ret->bcount, sizeof(bool));

    return (ret);
}


static inline void
t_strbuffer_add(struct t_strbuffer *sb, const char *s,
        size_t len, bool mine)
{
    assert_not_null(sb);
    assert(sb->bcount >= sb->blast);
    assert_not_null(s);

    if (sb->blast == (sb->bcount - 1)) {
    /* grow */
        sb->bcount  = sb->bcount * 2;
        sb->buffers = xrealloc(sb->buffers, sb->bcount * sizeof(char *));
        sb->blen    = xrealloc(sb->blen,    sb->bcount * sizeof(size_t));
        sb->bfree   = xrealloc(sb->bfree,   sb->bcount * sizeof(bool));
    }
    sb->blast += 1;
    sb->buffers[sb->blast] = s;
    sb->blen[sb->blast] = len;
    sb->bfree[sb->blast] = mine;
    sb->len += len;
}


static inline char *
t_strbuffer_get(struct t_strbuffer *sb)
{
    char *ret, *now;
    int i;

    assert_not_null(sb);
    assert(sb->bcount >= sb->blast);

    now = ret = xcalloc(sb->len + 1, sizeof(char));
    for (i = 0; i <= sb->blast; i++) {
        (void)memcpy(now, sb->buffers[i], sb->blen[i]);
        now += sb->blen[i];
    }

    t_strbuffer_destroy(sb);
    return (ret);
}


static inline void
t_strbuffer_destroy(struct t_strbuffer *sb)
{
    int i;

    assert_not_null(sb);

    for (i = sb->blast; i >= 0; i--) {
        if (sb->bfree[i])
            free((char *)(sb->buffers[i]));
    }
    freex(sb->buffers);
    freex(sb->blen);
    freex(sb->bfree);
    freex(sb);
}
#endif /* not T_STRBUFFER_H */

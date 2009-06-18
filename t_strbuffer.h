#ifndef T_STRBUFFER_H
#define T_STRBUFFER_H
/*
 * t_strbuffer.h
 *
 * String Buffer utils.
 *
 */
#include <stdlib.h>
#include <string.h> /* memcpy(3) */

#include "t_config.h"
#include "t_toolkit.h"


#define T_STRBUFFER_INIT_BCOUNT 8

struct t_strbuffer {
    char **buffers;
    int bcount, blast;
    size_t *blen, len;
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
 * the s pointer should be free()able and is owned by the t_strbuffer after the
 * call.
 */
_t__unused _t__nonnull(1) _t__nonnull(2)
static inline void t_strbuffer_add(struct t_strbuffer *restrict sb, char *s,
        size_t len);

/*
 * return the result of the buffer.
 *
 * returned value has to be free()d.
 */
_t__unused _t__nonnull(1)
static inline char * t_strbuffer_get(const struct t_strbuffer *restrict sb);

/*
 * free the given t_strbuffer.
 */
_t__unused _t__nonnull(1)
static inline void t_strbuffer_destroy(struct t_strbuffer *restrict sb);


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

    return (ret);
}


static inline void
t_strbuffer_add(struct t_strbuffer *restrict sb, char *s, size_t len)
{
    assert_not_null(sb);
    assert(sb->bcount >= sb->blast);
    assert_not_null(s);

    if (sb->blast == (sb->bcount - 1)) {
    /* grow */
        sb->bcount  = sb->bcount * 2;
        sb->buffers = xrealloc(sb->buffers, sb->bcount * sizeof(char *));
        sb->blen    = xrealloc(sb->blen,  sb->bcount * sizeof(size_t));
    }
    sb->blast += 1;
    sb->buffers[sb->blast] = s;
    sb->blen[sb->blast] = len;
    sb->len += len;
}


static inline char *
t_strbuffer_get(const struct t_strbuffer *restrict sb)
{
    char *ret, *now;
    int i;

    assert_not_null(sb);
    assert(sb->bcount >= sb->blast);

    now = ret = xcalloc(sb->len + 1, sizeof(char));
    for (i = 0; i <= sb->blast; i++) {
        memcpy(now, sb->buffers[i], sb->blen[i]);
        now += sb->blen[i];
    }

    return (ret);
}


static inline void
t_strbuffer_destroy(struct t_strbuffer *restrict sb)
{
    int i;

    assert_not_null(sb);

    for (i = sb->blast; i >= 0; i--)
        freex(sb->buffers[i]);
    freex(sb->buffers);
    freex(sb->blen);
    freex(sb);
}
#endif /* not T_STRBUFFER_H */

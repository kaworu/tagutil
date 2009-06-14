#ifndef T_TOOLKIT_H
#define T_TOOLKIT_H
/*
 * t_toolkit.h
 *
 * handy functions toolkit for tagutil.
 *
 */
#include "t_config.h"

#include </usr/include/assert.h>
#include <ctype.h>
#include <errno.h>
#include <err.h>
#include <libgen.h> /* dirname(3) */
#include <regex.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* compute the length of a fixed size array */
#define len(ary) (sizeof(ary) / sizeof((ary)[0]))

/* some handy assert macros */
#define assert_not_null(x) assert((x) != NULL)
#define assert_null(x) assert((x) == NULL)

/* error handling macros */
#define last_error_msg(o) ((o)->errmsg)
#define reset_error_msg(o) freex(last_error_msg(o))
#define set_error_msg(o, fmt, ...) \
    (void)xasprintf(&last_error_msg(o), fmt, ##__VA_ARGS__)


/* MEMORY FUNCTIONS */

_t__unused
static inline void * xmalloc(size_t size);

_t__unused
static inline void * xcalloc(size_t nmemb, const size_t size);

_t__unused
static inline void * xrealloc(void *ptr, size_t size);

#define freex(p) do { free(p); (p) = NULL; } while (/*CONSTCOND*/0)


/* FILE FUNCTIONS */

_t__unused _t__nonnull(1) _t__nonnull(2)
static inline FILE * xfopen(const char *restrict path, const char *restrict mode);

_t__unused _t__nonnull(1)
static inline void xfclose(FILE *restrict stream);

/*
 * try to have a sane dirname,
 * FreeBSD and OpenBSD define:
 *
 *      char * dirname(const char *)
 *
 * returned value has to be free()d.
 */
_t__unused _t__nonnull(1)
static inline char * xdirname(const char *restrict path);


/* BASIC STRING OPERATIONS */

_t__unused _t__nonnull(1)
static inline bool strempty(const char *restrict str);

_t__unused _t__nonnull(1)
static inline char * xstrdup(const char *restrict src);

_t__unused _t__printflike(2, 3)
static inline int xasprintf(char **ret, const char *fmt, ...);

/*
 * upperize a given string.
 */
_t__unused _t__nonnull(1)
static inline void strtoupper(char *restrict str);

/*
 * lowerize a given string.
 */
_t__unused _t__nonnull(1)
static inline void strtolower(char *restrict str);


/* BUFFER STRING OPERATIONS */

struct strbuf {
    char **buffers;
    int bcount, blast;
    size_t *blen, len;
};

/*
 * create and init a strbuf struct.
 */
_t__unused
static inline struct strbuf * new_strbuf(void);

/*
 * add the given (NUL-terminated) s to sb.
 * len should be strlen(s).
 *
 * the s pointer should be free()able and is owned by the strbuf after the
 * call.
 */
_t__unused _t__nonnull(1) _t__nonnull(2)
static inline void strbuf_add(struct strbuf *restrict sb, char *s, size_t len);

/*
 * return the result of the buffer.
 *
 * returned value has to be free()d.
 */
_t__unused _t__nonnull(1)
static inline char * strbuf_get(const struct strbuf *restrict sb);

/*
 * free the given strbuf.
 */
_t__unused _t__nonnull(1)
static inline void destroy_strbuf(struct strbuf *restrict sb);


/* OTHER */

/*
 * print the given question, and read user's input. input should match
 * y|yes|n|no.  yesno() loops until a valid response is given and then return
 * true if the response match y|yes, false if it match n|no.
 * Honor Yflag and Nflag.
 */
bool yesno(const char *restrict question);

/**********************************************************************/

static inline void *
xmalloc(size_t size)
{
    void *ptr;

    /*
     * we need to ensure that we request at least 1 byte, because malloc(0)
     * could return NULL and we err() if NULL is returned.
     */
    if (size == 0)
        size = 1;
    if ((ptr = malloc(size)) == NULL)
        err(ENOMEM, "malloc");

    return (ptr);
}


static inline void *
xcalloc(size_t nmemb, const size_t size)
{
    void *ptr;

    if (size == 0)
        err(EDOOFUS, "calloc(nmemb, 0)");
    if (nmemb == 0)
        nmemb = 1;
    if ((ptr = calloc(nmemb, size)) == NULL)
        err(ENOMEM, "calloc");

    return (ptr);
}


static inline void *
xrealloc(void *old_ptr, size_t new_size)
{
    void *ptr;

    if (new_size == 0)
        new_size = 1;
    if ((ptr = realloc(old_ptr, new_size)) == NULL)
        err(ENOMEM, "realloc");

    return (ptr);
}


static inline FILE *
xfopen(const char *restrict path, const char *restrict mode)
{
    FILE *stream;

    assert_not_null(path);
    assert_not_null(mode);

    stream = fopen(path, mode);

    if (stream == NULL)
        err(errno, "can't open file `%s'", path);

    return (stream);
}


static inline void
xfclose(FILE *restrict stream)
{

    assert_not_null(stream);

    if (fclose(stream) != 0)
        err(errno, "fclose");
}


static inline char *
xdirname(const char *restrict path)
{
    char *dirn;
#if !defined(HAVE_SANE_DIRNAME)
    char *garbage;
#endif

    assert_not_null(path);

#if !defined(HAVE_SANE_DIRNAME)
    if ((dirn = dirname(garbage = xstrdup(path))) == NULL)
#else
    if ((dirn = dirname(path)) == NULL)
#endif
        err(errno, "dirname");
    dirn = xstrdup(dirn);

#if !defined(HAVE_SANE_DIRNAME)
    freex(garbage);  /* no more needed */
#endif
    return (dirn);
}


static inline bool
strempty(const char *restrict str)
{

    assert_not_null(str);

    return (*str == '\0');
}


static inline char *
xstrdup(const char *restrict src)
{
    char *ret;

    if ((ret = strdup(src)) == NULL)
        err(ENOMEM, "strdup");

    return (ret);
}


static inline int
xasprintf(char **ret, const char *fmt, ...)
{
    int i;
    va_list ap;

    va_start(ap, fmt);
    i = vasprintf(ret, fmt, ap);
    if (i < 0)
        err(ENOMEM, "vasprintf");
    va_end(ap);

    return (i);
}


static inline void
strtoupper(char *restrict str)
{
    size_t len, i;

    assert_not_null(str);

    len = strlen(str);
    for (i = 0; i < len; i++)
        str[i] = toupper(str[i]);
}


static inline void
strtolower(char *restrict str)
{
    size_t len, i;

    assert_not_null(str);

    len = strlen(str);
    for (i = 0; i < len; i++)
        str[i] = tolower(str[i]);
}


static inline struct strbuf *
new_strbuf(void)
{
    struct strbuf *ret;

    ret = xmalloc(sizeof(struct strbuf));

    ret->len     =  0;
    ret->bcount  =  8;
    ret->blast   = -1;
    ret->buffers = xcalloc(ret->bcount, sizeof(char *));
    ret->blen    = xcalloc(ret->bcount, sizeof(size_t));

    return (ret);
}


static inline void
strbuf_add(struct strbuf *restrict sb, char *s, size_t len)
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
strbuf_get(const struct strbuf *restrict sb)
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
destroy_strbuf(struct strbuf *restrict sb)
{
    int i;
    assert_not_null(sb);

    for (i = sb->blast; i >= 0; i--)
        freex(sb->buffers[i]);
    freex(sb->buffers);
    freex(sb->blen);
    freex(sb);
}
#endif /* not T_TOOLKIT_H */

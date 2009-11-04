#ifndef T_TOOLKIT_H
#define T_TOOLKIT_H
/*
 * t_toolkit.h
 *
 * handy functions toolkit for tagutil.
 *
 */
#include </usr/include/assert.h>
#include <ctype.h>
#include <errno.h>
#include <err.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* mkstemp(3) */

#include "t_config.h"


/* compute the length of a fixed size array */
#define countof(ary) (sizeof(ary) / sizeof((ary)[0]))

/* No Operation */
#define NOP (void)0

/* some handy assert macros */
#define assert_not_null(x) assert((x) != NULL)
#define assert_null(x) assert((x) == NULL)
#define assert_fail() abort()

/* taken from FreeBSD's <sys/cdefs.h> */
#define	T_CONCAT1(x,y)  x ## y
#define	T_CONCAT(x,y)   T_CONCAT1(x,y)
#define	T_STRING(x)     #x /* stringify without expanding x */
#define	T_XSTRING(x)    T_STRING(x)	/* expand x, then stringify */


/* MEMORY FUNCTIONS */

_t__unused
static inline void * xmalloc(size_t size);

_t__unused
static inline void * xcalloc(size_t nmemb, size_t size);

_t__unused
static inline void * xrealloc(void *ptr, size_t size);

#define freex(p) do { free(p); (p) = NULL; } while (/*CONSTCOND*/0)


/* FILE FUNCTIONS */

_t__unused _t__nonnull(1) _t__nonnull(2)
static inline FILE * xfopen(const char *restrict path, const char *restrict mode);

_t__unused _t__nonnull(1)
static inline void xfclose(FILE *restrict stream);

_t__unused _t__nonnull(1)
static inline void xunlink(const char *restrict path);

/*
 * create a temporary file in $TMPDIR. if $TMPDIR is not set, /tmp is
 * used. return the full path to the temp file created.
 *
 * returned value has to be free()d.
 */
_t__unused _t__nonnull(1)
static inline char * t_mkstemp(const char *restrict dir);

/* BASIC STRING OPERATIONS */

_t__unused _t__nonnull(1)
static inline bool t_strempty(const char *restrict str);

_t__unused _t__nonnull(1)
static inline char * xstrdup(const char *restrict src);

_t__unused _t__printflike(2, 3)
static inline int xasprintf(char **ret, const char *fmt, ...);

/*
 * upperize a given string.
 */
_t__unused _t__nonnull(1)
static inline char * t_strtoupper(char *restrict str);

/*
 * lowerize a given string.
 */
_t__unused _t__nonnull(1)
static inline char * t_strtolower(char *restrict str);


/* OTHER */

/*
 * print the given question, and read user's input. input should match
 * y|yes|n|no.  t_yesno() loops until a valid response is given and then return
 * true if the response match y|yes, false if it match n|no.
 * Honor Yflag and Nflag.
 */
bool t_yesno(const char *restrict question);

/*
 * reentrant dirname.
 *
 * returned value has to be free()d.
 */
_t__unused
char * t_dirname(const char *);

/**********************************************************************/

static inline void *
xmalloc(size_t size)
{
    void *ptr;

    /*
     * we need to ensure that we request at least 1 byte, because malloc(0)
     * could return NULL and we err() if NULL is returned.
     */
    if (size == 0) {
        warnx("xmalloc: xmalloc(0)");
        size = 1;
    }
    if ((ptr = malloc(size)) == NULL)
        err(ENOMEM, "malloc");

    return (ptr);
}


#if !defined(EDOOFUS)
#define	EDOOFUS		42		/* Programming error */
#endif
static inline void *
xcalloc(size_t nmemb, size_t size)
{
    void *ptr;

    if (size == 0)
        err(EDOOFUS, "xcalloc(?, 0)");
    if (nmemb == 0) {
        warnx("xcalloc: xcalloc(0, ?)");
        size = nmemb = 1;
    }
    if ((ptr = calloc(nmemb, size)) == NULL)
        err(ENOMEM, "calloc");

    return (ptr);
}


static inline void *
xrealloc(void *old_ptr, size_t new_size)
{
    void *ptr;

    if (new_size == 0) {
        free(old_ptr);
        return (NULL);
    }

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


static inline void
xunlink(const char *restrict path)
{
    assert_not_null(path);

    if (unlink(path) != 0)
        err(errno, "unlink");
}


static inline char *
t_mkstemp(const char *restrict dir)
{
    char *f;

    assert_not_null(dir);

    (void)xasprintf(&f, "%s/%s-XXXXXX", dir, getprogname());

    if (mkstemp(f) == -1)
        err(errno, "mkstemp(\"%s\")", f);

    return (f);
}


static inline bool
t_strempty(const char *restrict str)
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

    assert_not_null(ret);

    va_start(ap, fmt);
    i = vasprintf(ret, fmt, ap);
    if (i < 0)
        err(ENOMEM, "vasprintf");
    va_end(ap);

    return (i);
}


static inline char *
t_strtoupper(char *restrict str)
{
    size_t len, i;

    assert_not_null(str);

    len = strlen(str);
    for (i = 0; i < len; i++)
        str[i] = toupper(str[i]);

    return (str);
}


static inline char *
t_strtolower(char *restrict str)
{
    size_t len, i;

    assert_not_null(str);

    len = strlen(str);
    for (i = 0; i < len; i++)
        str[i] = tolower(str[i]);

    return (str);
}

#endif /* not T_TOOLKIT_H */

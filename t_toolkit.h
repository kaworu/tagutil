#ifndef T_TOOLKIT_H
#define T_TOOLKIT_H
/*
 * t_toolkit.h
 *
 * handy functions toolkit for tagutil.
 *
 */
#include "t_config.h"

#include <assert.h>
#include <errno.h>
#include <err.h>
#include <libgen.h> /* dirname(3) */
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* compute the length of a fixed size array */
#define len(ary) (sizeof(ary) / sizeof((ary)[0]))

/* some handy assert macros */
#define assert_not_null(x) assert((x) != NULL)
#define assert_null(x) assert((x) == NULL)

/* true if the given string is empty (has only whitespace char) */
#define empty_line(l) has_match((l), "^\\s*$")


/* MEMORY FUNCTIONS */

__t__unused
static inline void * xmalloc(const size_t size);

__t__unused
static inline void * xcalloc(const size_t nmemb, const size_t size);

__t__unused
static inline void * xrealloc(void *ptr, const size_t size);


/* FILE FUNCTIONS */

__t__unused __t__nonnull(1) __t__nonnull(2)
static inline FILE * xfopen(const char *restrict path, const char *restrict mode);

__t__unused __t__nonnull(1)
static inline void xfclose(FILE *restrict stream);

__t__unused __t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
bool xgetline(char **line, size_t *size, FILE *restrict stream);

/*
 * try to have a sane dirname,
 * FreeBSD and OpenBSD define:
 *
 *      char * dirname(const char *)
 *
 * returned value has to be freed.
 */
__t__unused __t__nonnull(1)
static inline char * xdirname(const char *restrict path);


/* BASIC STRING OPERATIONS */

__t__unused __t__nonnull(1)
static inline char * xstrdup(const char *restrict src);

/*
 * copy src at the end of dest. dest_size is the allocated size
 * of dest and is modified (dest is realloc).
 *
 * returned value has to be freed.
 */
__t__unused __t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
static inline void concat(char **dest, size_t *destlen, const char *src);


/* REGEX STRING OPERATIONS */

/*
 * compile the given pattern, match it with str and return the regmatch_t result.
 * if an error occure (during the compilation or the match), print the error message
 * and err(3). return NULL if there was no match, and a pointer to the regmatch_t
 * otherwise.
 *
 * returned value has to be freed.
 */
__t__unused __t__nonnull(1) __t__nonnull(2)
regmatch_t * first_match(const char *restrict str, const char *restrict pattern, const int flags);

/*
 * return true if the regex pattern match the given str, false otherwhise.
 * flags are REG_ICASE | REG_EXTENDED | REG_NEWLINE | REG_NOSUB (see regex(3)).
 */
__t__unused __t__nonnull(2)
bool has_match(const char *restrict str, const char *restrict pattern);

/*
 * match pattern to str. then copy the match part and return the fresh
 * new char * created.
 * flags are REG_ICASE | REG_EXTENDED | REG_NEWLINE (see regex(3)).
 * get_match() can return NULL if no match was found.
 *
 * returned value has to be freed.
 */
__t__unused __t__nonnull(1) __t__nonnull(2)
char * get_match(const char *restrict str, const char *restrict pattern);

/*
 * replace the part defined by match in str by replace. for example
 * sub_match("foo bar", {.rm_so=3, .rm_eo=6}, "oni") will return
 * "foo oni". sub_match doesn't modify it's arguments, it return a new string.
 *
 * returned value has to be freed.
 */
__t__unused __t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
char * sub_match(const char *str, const regmatch_t *restrict match, const char *replace);

/*
 * same as sub_match but change the string reference given by str. *str will
 * be freed so it has to be malloc'd previously.
 */
__t__unused __t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
void inplacesub_match(char **str, regmatch_t *restrict match, const char *replace);


/* OTHER */

/*
 * print the given question, and read user's input. input should match
 * y|yes|n|no.  yesno() loops until a valid response is given and then return
 * true if the response match y|yes, false if it match n|no.
 */
__t__unused
bool yesno(const char *restrict question);

/*
 * return the program's name.
 */
#if defined(T_LACK_OF_GETPROGNAME)
__t__unused
static inline const char * getprogname(void);
#endif

/**********************************************************************/

static inline void *
xmalloc(const size_t size)
{
    void *ptr;

    if ((ptr = malloc(size)) == NULL)
        err(ENOMEM, NULL);

    return (ptr);
}


static inline void *
xcalloc(const size_t nmemb, const size_t size)
{
    void *ptr;

    if ((ptr = calloc(nmemb, size)) == NULL)
        err(ENOMEM, NULL);

    return (ptr);
}


static inline void *
xrealloc(void *old_ptr, const size_t new_size)
{
    void *ptr;

    if ((ptr = realloc(old_ptr, new_size)) == NULL)
        err(ENOMEM, NULL);

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
        err(errno, "can't open file %s", path);

    return (stream);
}


static inline void
xfclose(FILE *restrict stream)
{

    assert_not_null(stream);

    if (fclose(stream) != 0)
        err(errno, NULL);
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
        err(errno, NULL);
    dirn = xstrdup(dirn);

#if !defined(HAVE_SANE_DIRNAME)
    free(garbage);  /* no more needed */
#endif
    return (dirn);
}


static inline char *
xstrdup(const char *restrict src)
{
    char *ptr;

    if ((ptr = strdup(src)) == NULL)
        err(ENOMEM, NULL);

    return (ptr);
}


static inline void
concat(char **dest, size_t *dest_size, const char *src)
{
    size_t start, src_size, final_size;

    assert_not_null(dest);
    assert_not_null(*dest);
    assert_not_null(dest_size);
    assert_not_null(src);

    start       = *dest_size - 1;
    src_size    = strlen(src);
    final_size  = *dest_size + src_size;

    if (src_size == 0)
        return;

    *dest = xrealloc(*dest, final_size);

    memcpy(*dest + start, src, src_size + 1);
    *dest_size = final_size;
}

#endif /* not T_TOOLKIT_H */

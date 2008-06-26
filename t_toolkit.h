#ifndef T_TOOLKIT_H
#define T_TOOLKIT_H
/*
 * t_toolkit.h
 *
 * handy functions toolkit for tagutil.
 *
 */

#include <assert.h>
#include <errno.h>
#include <err.h>    /* porting: These functions are non-standard BSD extensions. */
#include <libgen.h> /* for dirname() POSIX.1-2001 */
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"


/* compute the length of a fixed size array */
#define len(ary) (sizeof(ary) / sizeof((ary)[0]))

/* some handy assert macros */
#define assert_not_null(x) assert((x) != NULL)
#define assert_null(x) assert((x) == NULL)

/* true if the given string is empty (has only whitespace char) */
#define empty_line(l) has_match((l), "^\\s*$")


/* MEMORY FUNCTIONS */

static inline void* xmalloc(const size_t size)
    __attribute__ ((__malloc__, __unused__));


static inline void* xcalloc(const size_t nmemb, const size_t size)
    __attribute__ ((__malloc__, __unused__));


static inline void* xrealloc(void *old_ptr, const size_t new_size)
    __attribute__ ((__malloc__, __unused__));


/* FILE FUNCTIONS */
static inline FILE* xfopen(const char *restrict path, const char *restrict mode)
    __attribute__ ((__nonnull__ (1, 2), __unused__));


static inline void xfclose(FILE *restrict fp)
    __attribute__ ((__nonnull__ (1), __unused__));


bool xgetline(char **line, size_t *size, FILE *restrict fp)
    __attribute__ ((__nonnull__ (1, 2, 3), __unused__));


/*
 * try to have a *sane* dirname() function. dirname() implementation may change argument.
 *
 * returned value has to be freed.
 */
static inline char* xdirname(const char *restrict path)
    __attribute__ ((__nonnull__ (1), __unused__));


/* BASIC STRING OPERATIONS */

static inline char* xstrdup(const char *restrict src)
    __attribute__ ((__nonnull__ (1), __unused__));


/*
 * copy src at the end of dest. dest_size is the allocated size
 * of dest and is modified (dest is realloc).
 *
 * returned value has to be freed.
 */
static inline void concat(char **dest, size_t *dest_size, const char *src)
    __attribute__ ((__nonnull__ (1, 2, 3), __unused__));


/* REGEX STRING OPERATIONS */

/*
 * compile the given pattern, match it with str and return the regmatch_t result.
 * if an error occure (during the compilation or the match), print the error message
 * and err(3). return NULL if there was no match, and a pointer to the regmatch_t
 * otherwise.
 *
 * returned value has to be freed.
 */
regmatch_t* first_match(const char *restrict str, const char *restrict pattern, const int flags)
    __attribute__ ((__nonnull__ (1, 2), __unused__));


/*
 * return true if the regex pattern match the given str, false otherwhise.
 * flags are REG_ICASE | REG_EXTENDED | REG_NEWLINE | REG_NOSUB (see regex(3)).
 */
bool has_match(const char *restrict str, const char *restrict pattern)
    __attribute__ ((__nonnull__ (2), __unused__));


/*
 * match pattern to str. then copy the match part and return the fresh
 * new char* created.  * flags are REG_ICASE | REG_EXTENDED | REG_NEWLINE
 * (see regex(3)).
 * get_match() can return NULL if no match was found, but when a match occur
 *
 * returned value has to be freed.
 */
char* get_match(const char *restrict str, const char *restrict pattern)
    __attribute__ ((__nonnull__ (1, 2), __unused__));


/*
 * replace the part defined by match in str by replace. for example
 * sub_match("foo bar", {.rm_so=3, .rm_eo=6}, "oni") will return
 * "foo oni". sub_match doesn't modify it's arguments, it return a new string.
 *
 * returned value has to be freed.
 */
char* sub_match(const char *str, const regmatch_t *restrict match, const char *replace)
    __attribute__ ((__nonnull__ (1, 2, 3), __unused__));


/*
 * same as sub_match but change the string reference given by str. *str will
 * be freed so it has to be malloc'd previously.
 */
void inplacesub_match(char **str, regmatch_t *restrict match, const char *replace)
    __attribute__ ((__nonnull__ (1, 2, 3), __unused__));


/* OTHER */

/*
 * print the given question, and read user's input. input should match
 * y|yes|n|no.  yesno() loops until a valid response is given and then return
 * true if the response match y|yes, false if it match n|no.
 */
bool yesno(const char *restrict question)
    __attribute__ ((__unused__));

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
    FILE *fp;

    assert_not_null(path);
    assert_not_null(mode);

    fp = fopen(path, mode);

    if (fp == NULL)
        err(errno, "can't open file %s", path);

    return (fp);
}


static inline void
xfclose(FILE *restrict fp)
{

    assert_not_null(fp);

    if (fclose(fp) != 0)
        err(errno, NULL);
}


static inline char *
xdirname(const char *restrict path)
{
    char *dirn;
#if defined(WITH_INSANE_DIRNAME)
    char *garbage;
#endif

    assert_not_null(path);

#if defined(WITH_INSANE_DIRNAME)
    dirn = dirname(garbage = xstrdup(path));
    free(garbage);  /* no more needed */
#else
    dirn = dirname(path);
#endif

    if (dirn == NULL)
        err(errno, NULL);

    return (xstrdup(dirn));
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

    *dest = xrealloc(*dest, final_size);

    memcpy(*dest + start, src, src_size + 1);
    *dest_size = final_size;
}

#endif /* !T_TOOLKIT_H */

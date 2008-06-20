#ifndef T_TOOLKIT_H
#define T_TOOLKIT_H
/*
 * t_toolkit.h
 *
 * handy functions toolkit for tagutil.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <err.h>    /* These functions are non-standard BSD extensions. */
#include <libgen.h> /* for dirname() POSIX.1-2001 */

#include "config.h"


/* compute the length of a fixed size array */
#define len(ary) (sizeof(ary) / sizeof((ary)[0]))

#define assert_not_null(x) assert((x) != NULL)
#define assert_null(x) assert((x) == NULL)

/* true if the given string is empty (has only whitespace char) */
#define EMPTY_LINE(l) has_match((l), "^\\s*$")

#if !defined(NDEBUG)
/*
 * count how many times we call x*alloc. if needed, print their value at
 * the end of main. I used them to see how many alloc tagutil does, to see
 * the ratio of alloc (tagutil VS taglib). It seems that tagutil make about 1%
 * of all the memory allocation (in alloc/free call and size).
 */
int _alloc_counter;
int _alloc_b;
#define INIT_ALLOC_COUNTER      \
    do {                        \
        _alloc_counter = 0;     \
        _alloc_b = 0;           \
    } while (/*CONSTCOND*/0)
#define INCR_ALLOC_COUNTER(x)   \
    do {                        \
        _alloc_counter++;       \
        _alloc_b += (x);        \
    } while (/*CONSTCOND*/0)
#define SHOW_ALLOC_COUNTER (void) fprintf(stderr, "bytes alloc: %d, alloc calls: %d\n", _alloc_b, _alloc_counter)
#else /* NDEBUG */
#define INIT_ALLOC_COUNTER
#define INCR_ALLOC_COUNTER
#define SHOW_ALLOC_COUNTER
#endif /* !NDEBUG */

#if !defined(NDEBUG)
#endif


/* MEMORY FUNCTIONS */

static __inline__ void* xmalloc(const size_t size)
    __attribute__ ((__malloc__, __unused__));


static __inline__ void* xcalloc(const size_t nmemb, const size_t size)
    __attribute__ ((__malloc__, __unused__));


static __inline__ void* xrealloc(void *old_ptr, const size_t new_size)
    __attribute__ ((__malloc__, __unused__));


/* FILE FUNCTIONS */
static __inline__ FILE* xfopen(const char *__restrict__ path, const char *__restrict__ mode)
    __attribute__ ((__nonnull__ (1, 2), __unused__));


static __inline__ void xfclose(FILE *__restrict__ fp)
    __attribute__ ((__nonnull__ (1), __unused__));


bool xgetline(char **line, size_t *size, FILE *__restrict__ fp)
    __attribute__ ((__nonnull__ (1, 2, 3), __unused__));


/*
 * try to have a *sane* dirname() function. dirname() implementation may change argument.
 *
 * returned value has to be freed.
 */
static __inline__ char* xdirname(const char *__restrict__ path)
    __attribute__ ((__nonnull__ (1), __unused__));


/* BASIC STRING OPERATIONS */

/*
 * make a copy of src.
 *
 * returned value has to be freed.
 */
static __inline__ char* str_copy(const char *__restrict__ src)
    __attribute__ ((__nonnull__ (1), __unused__));


/*
 * copy src at the end of dest. dest_size is the allocated size
 * of dest and is modified (dest is realloc).
 *
 * returned value has to be freed.
 */
static __inline__ void concat(char **dest, size_t *dest_size, const char *src)
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
regmatch_t* first_match(const char *__restrict__ str, const char *__restrict__ pattern, const int flags)
    __attribute__ ((__nonnull__ (1, 2), __unused__));


/*
 * return true if the regex pattern match the given str, false otherwhise.
 * flags are REG_ICASE | REG_EXTENDED | REG_NEWLINE | REG_NOSUB (see regex(3)).
 */
bool has_match(const char *__restrict__ str, const char *__restrict__ pattern)
    __attribute__ ((__nonnull__ (2), __unused__));


/*
 * match pattern to str. then copy the match part and return the fresh
 * new char* created.  * flags are REG_ICASE | REG_EXTENDED | REG_NEWLINE
 * (see regex(3)).
 * get_match() can return NULL if no match was found, but when a match occur
 *
 * returned value has to be freed.
 */
char* get_match(const char *__restrict__ str, const char *__restrict__ pattern)
    __attribute__ ((__nonnull__ (1, 2), __unused__));


/*
 * replace the part defined by match in str by replace. for example
 * sub_match("foo bar", {.rm_so=3, .rm_eo=6}, "oni") will return
 * "foo oni". sub_match doesn't modify it's arguments, it return a new string.
 *
 * returned value has to be freed.
 */
char* sub_match(const char *str, const regmatch_t *__restrict__ match, const char *replace)
    __attribute__ ((__nonnull__ (1, 2, 3), __unused__));


/*
 * same as sub_match but change the string reference given by str. *str will
 * be freed so it has to be malloc'd previously.
 */
void inplacesub_match(char **str, regmatch_t *__restrict__ match, const char *replace)
    __attribute__ ((__nonnull__ (1, 2, 3), __unused__));


/* OTHER */

/*
 * print the given question, and read user's input. input should match
 * y|yes|n|no.  yesno() loops until a valid response is given and then return
 * true if the response match y|yes, false if it match n|no.
 */
bool yesno(const char *__restrict__ question)
    __attribute__ ((__unused__));

/**********************************************************************/

static __inline__ void *
xmalloc(const size_t size)
{
    void *ptr;

    INCR_ALLOC_COUNTER(size);

    ptr = malloc(size);
    if (size > 0 && ptr == NULL)
        err(ENOMEM, NULL);

    return (ptr);
}


static __inline__ void *
xcalloc(const size_t nmemb, const size_t size)
{
    void *ptr;

    INCR_ALLOC_COUNTER(nmemb * size);

    ptr = calloc(nmemb, size);
    if (ptr == NULL && size > 0 && nmemb > 0)
        err(ENOMEM, NULL);

    return (ptr);
}


static __inline__ void *
xrealloc(void *old_ptr, const size_t new_size)
{
    void *ptr;

    INCR_ALLOC_COUNTER(new_size);

    ptr = realloc(old_ptr, new_size);
    if (ptr == NULL && new_size > 0)
        err(ENOMEM, NULL);

    return (ptr);
}


static __inline__ FILE *
xfopen(const char *__restrict__ path, const char *__restrict__ mode)
{
    FILE *fp;

    assert(path != NULL);
    assert(mode != NULL);

    fp = fopen(path, mode);

    if (fp == NULL)
        err(errno, "can't open file %s", path);

    return (fp);
}


static __inline__ void
xfclose(FILE *__restrict__ fp)
{

    assert(fp != NULL);

    if (fclose(fp) != 0)
        err(errno, NULL);
}


static __inline__ char *
xdirname(const char *__restrict__ path)
{
    char *pathcpy, *dirncpy;
    const char *dirn;
    size_t size;

    assert(path != NULL);

    /* we need to copy path, because dirname() can change the path argument. */
    size = strlen(path) + 1;
    pathcpy = xcalloc(size, sizeof(char));
    memcpy(pathcpy, path, size);

    dirn = dirname(pathcpy);

    /* copy dirname to dirnamecpy */
    size = strlen(dirn) + 1;
    dirncpy = xcalloc(size, sizeof(char));
    memcpy(dirncpy, dirn, size);

    free(pathcpy);

    return (dirncpy);
}


static __inline__ char *
str_copy(const char *__restrict__ src)
{
    char *result;
    size_t len;

    assert(src != NULL);

    len = strlen(src);
    result = xcalloc(len + 1, sizeof(char));
    strncpy(result, src, len);

    return (result);
}


static __inline__ void
concat(char **dest, size_t *dest_size, const char *src)
{
    size_t start, src_size, final_size;

    assert(dest         != NULL);
    assert(*dest        != NULL);
    assert(dest_size    != NULL);
    assert(src          != NULL);

    start       = *dest_size - 1;
    src_size    = strlen(src);
    final_size  = *dest_size + src_size;

    *dest = xrealloc(*dest, final_size);

    memcpy(*dest + start, src, src_size + 1);
    *dest_size = final_size;
}

#endif /* !T_TOOLKIT_H */

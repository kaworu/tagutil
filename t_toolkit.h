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
static inline regmatch_t * first_match(const char *restrict str, const char *restrict pattern, const int flags);

/*
 * return true if the regex pattern match the given str, false otherwhise.
 * flags are REG_ICASE | REG_EXTENDED | REG_NEWLINE | REG_NOSUB (see regex(3)).
 */
__t__unused __t__nonnull(2)
static inline bool has_match(const char *restrict str, const char *restrict pattern);


/* OTHER */

/*
 * print the given question, and read user's input. input should match
 * y|yes|n|no.  yesno() loops until a valid response is given and then return
 * true if the response match y|yes, false if it match n|no.
 */
__t__unused
static inline bool yesno(const char *restrict question);

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


static inline regmatch_t *
first_match(const char *restrict str, const char *restrict pattern, const int flags)
{
    regex_t regex;
    regmatch_t *regmatch;
    int error;
    char *errbuf;

    assert_not_null(str);
    assert_not_null(pattern);

    regmatch = xmalloc(sizeof(regmatch_t));
    error = regcomp(&regex, pattern, flags);

    if (error != 0)
        goto error_label;

    error = regexec(&regex, str, 1, regmatch, 0);
    regfree(&regex);

    switch (error) {
    case 0:
        return (regmatch);
    case REG_NOMATCH:
        free(regmatch);
        return (NULL);
    default:
error_label:
        errbuf = xcalloc(BUFSIZ, sizeof(char));
        (void)regerror(error, &regex, errbuf, BUFSIZ);
        errx(-1, "%s", errbuf);
        /* NOTREACHED */
    }
}


static inline bool
has_match(const char *restrict str, const char *restrict pattern)
{
    regmatch_t *match;

    assert_not_null(pattern);

    if (str == NULL)
        return (false);

    match = first_match(str, pattern, REG_ICASE | REG_EXTENDED | REG_NEWLINE | REG_NOSUB);

    if (match == NULL)
        return (false);
    else {
        free(match);
        return (true);
    }
}


static inline bool
yesno(const char *restrict question)
{
    char buffer[5]; /* strlen("yes\n\0") == 5 */

    for (;;) {
        if (feof(stdin))
            return (false);

        (void)memset(buffer, '\0', len(buffer));

        if (question != NULL)
            (void)printf("%s", question);
        (void)printf("? [y/n] ");

        (void)fgets(buffer, len(buffer), stdin);

        /* if any, eat stdin characters that didn't fit into buffer */
        if (buffer[strlen(buffer) - 1] != '\n') {
            while (getc(stdin) != '\n' && !feof(stdin))
                ;
        }

        if (has_match(buffer, "^(n|no)$"))
            return (false);
        else if (has_match(buffer, "^(y|yes)$"))
            return (true);
    }
}
#endif /* not T_TOOLKIT_H */

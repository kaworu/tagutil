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
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "t_config.h"


/* compute the length of a fixed size array */
#define countof(ary) (sizeof(ary) / sizeof((ary)[0]))

/* some handy assert macros */
#define assert_not_null(x) assert((x) != NULL)
#define assert_null(x) assert((x) == NULL)
#define assert_fail() abort()


t__unused
static inline void * xmalloc(size_t size);

t__unused
static inline void * xcalloc(size_t nmemb, size_t size);

t__unused
static inline void * xrealloc(void *ptr, size_t size);

#define freex(p) do { free(p); (p) = NULL; } while (/*CONSTCOND*/0)


t__unused
static inline char * xstrdup(const char *src);

t__unused t__printflike(2, 3)
static inline int xasprintf(char **ret, const char *fmt, ...);

/*
 * upperize a given string.
 */
t__unused
static inline char * t_strtoupper(char *str);

/*
 * lowerize a given string.
 */
t__unused
static inline char * t_strtolower(char *str);


/* OTHER */

/*
 * print the given question, and read user's input. input should match
 * y|yes|n|no.  t_yesno() loops until a valid response is given and then return
 * true if the response match y|yes, false if it match n|no.
 * Honor Yflag and Nflag.
 */
bool t_yesno(const char *question);

/*
 * call "$EDITOR path" (the environment variable $EDITOR must be set).
 */
bool t_user_edit(const char *path);

/*
 * dirname() routine that does not modify its argument.
 */
char * t_dirname(const char *);
/*
 * basename() routine that does not modify its argument.
 */
char * t_basename(const char *);

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
#define	EDOOFUS		88		/* Programming error */
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


static inline char *
xstrdup(const char *src)
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
t_strtoupper(char *str)
{
	char *s;

	assert_not_null(str);

	for (s = str; *s != '\0'; s++)
		*s = toupper(*s);
	return (str);
}


static inline char *
t_strtolower(char *str)
{
	char *s;

	assert_not_null(str);

	for (s = str; *s != '\0'; s++)
		*s = tolower(*s);
	return (str);
}

#endif /* not T_TOOLKIT_H */

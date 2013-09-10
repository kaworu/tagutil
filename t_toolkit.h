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


/*
 * upperize a given string.
 */
char * t_strtoupper(char *str);

/*
 * lowerize a given string.
 */
t__unused
char * t_strtolower(char *str);

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

#endif /* not T_TOOLKIT_H */

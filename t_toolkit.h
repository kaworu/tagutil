#ifndef T_TOOLKIT_H
#define T_TOOLKIT_H
/*
 * t_toolkit.h
 *
 * handy functions toolkit for tagutil.
 *
 */

/* FLAC's assert header conflict with the standard. It has been fixed in
 * "recent" libFLAC (by requiring prefixing) but not all OS / distro have
 * updated yet (namely Ubuntu 13.04 for example).
 */
#include </usr/include/assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "t_config.h"


/* compute the length of a fixed size array */
#define NELEM(ary) (sizeof(ary) / sizeof((ary)[0]))

/* some handy assert macros */
#define	assert_not_null(x) assert((x) != NULL)
#define	assert_null(x) assert((x) == NULL)
#define	ABANDON_SHIP() abort()


/*
 * upperize a given string.
 */
char	*t_strtoupper(char *str);

/*
 * lowerize a given string.
 */
char	*t_strtolower(char *str);

/*
 * print the given question, and read user's input. input should match
 * y|yes|n|no.  t_yesno() loops until a valid response is given and then return
 * 1 if the response match y|yes, 0 if it match n|no.
 * Honor Yflag and Nflag.
 */
int	t_yesno(const char *question);

/*
 * call "$EDITOR path" (the environment variable $EDITOR must be set).
 */
int	t_user_edit(const char *path);

/*
 * dirname() routine that does not modify its argument.
 */
char	*t_dirname(const char *);
/*
 * basename() routine that does not modify its argument.
 */
char	*t_basename(const char *);

#endif /* not T_TOOLKIT_H */

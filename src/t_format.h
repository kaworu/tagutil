#ifndef T_FORMAT_H
#define T_FORMAT_H
/*
 * t_format.h
 *
 * output formats functions for tagutil. Very similar to t_backend.
 */
#include <stdio.h>

#include "t_config.h"
#include "t_taglist.h"


struct t_format {
	const char	*libid;
	const char	*fileext;
	const char	*desc;

	/*
	 * return a formated string with all the given tags.
	 *
	 * @param tlist
	 *   The t_taglist to translate, cannot be NULL.
	 *
	 * @param path
	 *   Some format may output the path as a comment or as part of the
	 *   data.
	 *
	 * @return
	 *   A C-string containing data that must be passed to free(3) after
	 *   use. On error, NULL is returned and errno is set to ENOMEM.
	 */
	char	*(*tags2fmt)(const struct t_taglist *tlist, const char *path);

	/*
	 * Parse a format file and create a t_taglist based on its content.
	 *
	 * @param fp
	 *   a file pointer to the input stream that will passed to the parser.
	 *   A parser SHOULD NOT consume more that it needs (like until EOF)
	 *   because more than one pass could be used from the same fp.
	 *
	 * @param errmsg_p
	 *   a pointer to an error message. If not NULL and an error arise, it will be
	 *   set to a string with a descriptive error message. The error message string
	 *   should be passed to free(3) after use. If an error occured (NULL is
	 *   returned) and errmsg_p is NULL too, malloc(3) failed.
	 *
	 * @return
	 *   a t_taglist that should be passed to t_taglist_delete() after use. On
	 *   error, NULL is returned and the errmsg_p is set.
	 */
	struct t_taglist	*(*fmt2tags)(FILE *fp, char **errmsg_p);

	TAILQ_ENTRY(t_format)	entries;
};
TAILQ_HEAD(t_formatQ, t_format);

/*
 * initialize formats if needed and return the list.
 */
const struct t_formatQ	*t_all_formats(void);

#endif /* ndef T_FORMAT_H */

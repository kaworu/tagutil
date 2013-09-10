#ifndef T_YAML_H
#define T_YAML_H
/*
 * t_yaml.h
 *
 * yaml tagutil parser and converter.
 */
#include <stdbool.h>
#include <stdio.h>

#include "t_config.h"
#include "t_file.h"

/*
 * return a yaml formated string with all the given tags.
 *
 * @param tlist
 *   The t_taglist to translate, cannot be NULL.
 *
 * @param path
 *   if not NULL, a YAML comment with the filename will be inserted before data.
 *
 * @return
 *   a string containing YAML data that must be passed to free(3) after use. On
 *   error, NULL is returned and errno is set to ENOMEM.
 */
char *	t_tags2yaml(const struct t_taglist *tlist, const char *path);


/*
 * Parse a yaml file and create a t_taglist based on its content.
 *
 * @param fp
 *   a file pointer to the input stream that will passed to the YAML parser.
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
struct t_taglist	*t_yaml2tags(FILE *fp, char **errmsg_p);

#endif /* not T_YAML_H */

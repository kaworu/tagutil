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
 *   error, NULL is returned.
 */
char *	t_tags2yaml(const struct t_taglist *tlist, const char *path);


/*
 * read a yaml file.
 *
 * On error t_error_msg(file) contains an error message and NULL is
 * returned. Otherwhise the loaded t_taglist is returned.
 *
 * returned value has to be free()d (see t_taglist_delete()).
 */
struct t_taglist * t_yaml2tags(struct t_file *file, FILE *stream);

#endif /* not T_YAML_H */

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
 * return a yaml formated string with all the given file's tags.
 *
 * returned value has to be free()d.
 */
_t__nonnull(1)
char * t_tags2yaml(struct t_file *restrict file);


/*
 * read a yaml file.
 *
 * On error t_error_msg(file) contains an error message and NULL is
 * returned. Otherwhise the loaded t_taglist is returned.
 *
 * returned value has to be free()d (see t_taglist_destroy()).
 */
_t__nonnull(1) _t__nonnull(2)
struct t_taglist * t_yaml2tags(struct t_file *restrict file,
        FILE *restrict stream);

#endif /* not T_YAML_H */

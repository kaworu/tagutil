#ifndef T_YAML_H
#define T_YAML_H
/*
 * t_yaml.h
 *
 * yaml tagutil parser and converter.
 */
#include "t_config.h"

#include <stdbool.h>
#include <stdio.h>

#include "t_file.h"

/*
 * return a yaml formated string with all the given file's tags.
 *
 * returned value has to be free()d.
 */
_t__nonnull(1)
char * tags_to_yaml(struct tfile *restrict file);


/*
 * read a yaml file.
 *
 * On error last_error_msg(file) contains an error message and NULL is
 * returned. Otherwhise the loaded tag_list is returned.
 *
 * returned value has to be free()d (see destroy_tag_list()).
 */
_t__nonnull(1) _t__nonnull(2)
struct tag_list * yaml_to_tags(struct tfile *restrict file,
        FILE *restrict stream);

#endif /* not T_YAML_H */

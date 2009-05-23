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
 * return a yaml formated string.
 *
 * # filename
 * ---
 * title:   "%t"
 * album:   "%a"
 * artist:  "%A"
 * year:    "%y"
 * track:   "%T"
 * comment: "%c"
 * genre:   "%g"
 *
 * '"' and '\' in tags are escaped to \" and \\
 */
_t__nonnull(1)
char * tags_to_yaml(const struct tfile *restrict file);


/*
 * read a yaml file and set the tag of given file accordingly.
 * return false if a problem occured, true otherwise.
 */
_t__nonnull(1) _t__nonnull(2)
bool yaml_to_tags(struct tfile *restrict file, FILE *restrict stream);

#endif /* not T_YAML_H */

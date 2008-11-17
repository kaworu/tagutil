/*
 * t_yaml.h
 *
 * YAML tagutil parser and converter.
 */
#include "t_config.h"

#include <stdbool.h>
#include <stdio.h>

#include <tag_c.h>

__t__nonnull(1) __t__nonnull(2)
char * tags_to_yaml(const char *restrict path,
        const TagLib_Tag *restrict tags);


__t__nonnull(1) __t__nonnull(2)
bool yaml_to_tags(TagLib_Tag *restrict tags, FILE *restrict stream);

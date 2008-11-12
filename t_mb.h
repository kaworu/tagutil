#ifndef T_MB_H
#define T_MB_H
/*
 * t_mb.h
 *
 * tagutil's MusicBrainz access lib.
 * glue methods for tagutil to t_http/t_xml.
 */

#include <taglib/tag_c.h>

#include "config.h"
#include "t_xml.h"

/* MusicBrainz duration response are in millisec. */
#define mb_duration_to_seconds(x) ((x) / 1000)

/*
 * query the MB server. arg is the argument encoded in a HTML way
 * /page.php?foo=bar&oni=42. It use MB_HOST and MB_PORT env vars or the
 * default values (so user can choose another web server that implement the
 * MusicBrainz interface) It return the server's response.
 *
 * returned value has to be freed.
 */
char* mb_get(const char *restrict arg);


/*
 * FIXME: @return shouldn't be void.
 */
void mb_choice(const TagLib_File *restrict f, const struct xml_tree *restrict tree)
    __attribute__ ((__nonnull__ (1, 2)));

#endif /* not T_MB_H */

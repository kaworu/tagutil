#ifndef T_MB_H
#define T_MB_H
/*
 * t_mb.h
 *
 * tagutil's MusicBrainz access lib.
 * glue methods for tagutil to t_http/t_xml.
 */

#include "config.h"

/*
 * query the MB server. arg is the argument encoded in a HTML way
 * ?foo=bar&oni=42. It use MB_HOST and MB_PORT env vars or the default values
 * defined at configure time.
 * It return the server's response.
 *
 * return value has to be freed.
 */
char* mb_get(const char *__restrict__ arg);
#endif /* !T_MB_H */

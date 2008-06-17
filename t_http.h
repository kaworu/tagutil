#ifndef T_NET_H
#define T_NET_H
/*
 * t_http.h
 *
 * HTTP request function for tagutil.
 */

#include "config.h"

/*
 * query the host server. arg (can be null) is the argument encoded in a
 * HTML way ?foo=bar&oni=42. It return the server's response.
 *
 * return value has to be freed.
 */
char* net_get(const char *__restrict__ host, const char *__restrict__ port, const char *__restrict__ arg)
    __attribute__ ((__nonnull__ (1, 2)));
#endif /* !T_NET_H */

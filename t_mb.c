/*
 * t_mb.h
 *
 * tagutil's MusicBrainz access lib.
 * glue methods for tagutil to t_http/t_xml.
 */

#include "t_mb.h"
#include "t_toolkit.h"
#include "t_http.h"
#include "t_xml.h"

char *
mb_get(const char *__restrict__ arg)
{
    char *host, *port;

    if ((host = getenv("MB_HOST")) == NULL)
        host = MB_DEFAULT_HOST;
    if ((port = getenv("MB_PORT")) == NULL)
        port = MB_DEFAULT_PORT;

    return (net_get(host, port, arg));
}

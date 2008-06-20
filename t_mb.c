/*
 * t_mb.h
 *
 * tagutil's MusicBrainz access lib.
 * glue methods for tagutil to t_http/t_xml.
 */
#include <taglib/tag_c.h>

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

    return (http_request(host, port, HTTP_GET, arg));
}

void mb_choice(void)
{

    /*
     * function to honor qsort(3)
     */
int mb_by_length_sorter(const void *x, const void *y)
{
    int xdelta, ydelta;

    xlength = atoi(xml_lookup(x, "duration")->content.data)
    ylength = atoi(xml_lookup(y, "duration")->content.data)

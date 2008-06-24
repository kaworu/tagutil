/*
 * t_mb.c
 *
 * tagutil's MusicBrainz access lib.
 * glue methods for tagutil to t_http/t_xml.
 */

#include <taglib/tag_c.h>

#include "config.h"
#include "t_mb.h"
#include "t_toolkit.h"
#include "t_http.h"
#include "t_xml.h"


/*
 * cmp function to honor qsort_r(3)
 * data is an int: the duration of the current track.
 * x and y two "track" xml_tree struct.
 */
static int mb_qsortr_cmp(void *data, const void *x, const void *y)
    __attribute__ ((__nonnull__ (1, 2, 3)));


char *
mb_get(const char *restrict arg)
{
    char *host, *port;

    if ((host = getenv("MB_HOST")) == NULL)
        host = MB_DEFAULT_HOST;
    if ((port = getenv("MB_PORT")) == NULL)
        port = MB_DEFAULT_PORT;

    return (http_request(host, port, HTTP_GET, arg));
}


void
mb_choice(const TagLib_File *restrict f, const struct xml_tree *restrict tree)
{
    struct xml_tree *tracklst, *current;
    int track_duration;

    assert_not_null(f);
    assert_not_null(tree);
    assert(strcmp(tree->name, "__root__") == 0);

    track_duration = taglib_audioproperties_length(taglib_file_audioproperties(f));

    tracklst = xml_reclookup(tree, 2, "metadata", "track-list");
    assert_not_null(tracklst);
    assert(tracklst->type == XML_NODE);
    assert(strcmp(tracklst->name, "track-list") == 0);
    assert(tracklst->content_len == atoi(xml_attrlookup(tracklst, "count")));

    qsort_r(tracklst->content.childv, tracklst->content_len, sizeof(struct xml_tree *), &track_duration, mb_qsortr_cmp);

    for (int i = 0; i < tracklst->content_len; i++) {
        current = tracklst->content.childv[i];
        (void)printf("%i> %s by %s [%is]\n", i,
                xml_lookup(current, "title")->content.data,
                xml_reclookup(current, 2, "artist", "name")->content.data,
                mb_duration_to_seconds(atoi(xml_lookup(current, "duration")->content.data)));
    }
}


static int mb_qsortr_cmp(void *data, const void *x, const void *y)
{
    int *track_duration;
    int xdelta, ydelta;
    int xduration, yduration;
    struct xml_tree *xtree, *ytree;

    assert_not_null(data);
    assert_not_null(x);
    assert_not_null(y);

    track_duration = (int *)data;
    xtree = *((struct xml_tree **)x);
    ytree = *((struct xml_tree **)y);
    xduration = mb_duration_to_seconds(atoi(xml_lookup(xtree, "duration")->content.data));
    yduration = mb_duration_to_seconds(atoi(xml_lookup(ytree, "duration")->content.data));
    xdelta = abs(xduration - *track_duration);
    ydelta = abs(yduration - *track_duration);

    return (xdelta - ydelta);
}

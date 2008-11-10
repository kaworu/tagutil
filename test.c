/*
 * test.c
 */
#include <stdio.h>

#include <taglib/tag_c.h>

#include "config.h"
#include "t_mb.h"
#include "t_xml.h"
#include "t_http.h"
#include "t_toolkit.h"

int main(void)
{
    char *data;
    struct xml_tree *t;
    FILE *fd;
    TagLib_File *f;

    /*data = http_request("128.178.251.135", "80", HTTP_GET, NULL); */
    data = malloc(sizeof(char) * BUFSIZ * 10);
    fd = fopen("dummy.xml", "r");
    fgets(data, BUFSIZ * 10, fd);
    fclose(fd);

    t = xml_parse(data);

    xml_tree_show(xml_reclookup(t, 3, "metadata", "track-list", "track"));
    fflush(stdout);

    f = taglib_file_new("song36.flac");
    mb_choice(f, t);

    taglib_tag_free_strings();
    taglib_file_free(f);

    free(data);
    xml_tree_delete(t);
    return (0);
}


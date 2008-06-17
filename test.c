/*
 * test.c
 */

#include <stdio.h>

#include "t_xml.h"
#include "t_net.h"
#include "t_toolkit.h"

int main(int argc, char *argv[])
{
    char *data;
    struct xml_tree *t;

    data = net_get("localhost", "80", NULL);
    t = xml_parse(data);

    xml_tree_show(t);

    free(data);
    xml_tree_delete(t);
    (void) printf("DEBUG: counter:%d, b:%d\n", _alloc_counter, _alloc_b);
    return (0);
}


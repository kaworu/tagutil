#ifndef T_XML_H
#define T_XML_H
/*
 * t_xml.h
 *
 * xml parser for tagutil.
 * use expat.
 */

#include "config.h"


struct xml_attr {
    char *key, *val;
};

typedef enum { XML_LEAF, XML_NODE } xml_trtype;

struct xml_tree {
     char *name;
    xml_trtype type;
    int attrc, depth;
    struct xml_attr **attrv;
    int content_len; /* if type is XML_LEAF it's the data string's length, otherwise its the number of childrens */
    union {
        char *data;                 /* type == XML_LEAF */
        struct xml_tree **childv;   /* type == XML_NODE */
    } content;
    struct xml_tree *parent;

#if !defined(NDEBUG)
    bool frozen;
#endif
};


/*
 * display a tree to stdout. debug.
 */
void xml_tree_show(const struct xml_tree *__restrict__ tree)
    __attribute__ ((__nonnull__ (1), __unused__));


/*
 * free a struct xml_tree recursively.
 */
void xml_tree_delete(struct xml_tree *__restrict__ victim)
    __attribute__ ((__nonnull__ (1)));


/*
 * parse the given xml string data and return a new
 * xml_tree struct.
 *
 * return value has to be freed.
 */
struct xml_tree* xml_parse(const char *__restrict__ data)
    __attribute__ ((__nonnull__ (1)));

#endif /* !T_XML_H */

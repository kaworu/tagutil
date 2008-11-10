#ifndef T_XML_H
#define T_XML_H
/*
 * t_xml.h
 *
 * xml parser for tagutil.
 * use expat.
 */

#include "config.h"

#define XML_ROOT "__root__"

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
#include <stdbool.h>
    bool frozen;
#endif
};


/*
 * create a new xml_tree struct under given parent. if parent is NULL, then
 * the new xml_tree struct is a root node.
 *
 * called by expat handlers.
 */
struct xml_tree* xml_tree_new(struct xml_tree *restrict parent, const char *restrict name) __attribute__ ((__nonnull__ (2)));


/*
 * parse the given xml string data and return a new
 * xml_tree struct.
 *
 * returned value has to be freed.
 */
struct xml_tree* xml_parse(const char *restrict data)
    __attribute__ ((__nonnull__ (1)));


/*
 * display a tree to stdout. debug.
 */
void xml_tree_show(const struct xml_tree *restrict tree)
    __attribute__ ((__nonnull__ (1), __unused__));


/*
 * free a struct xml_tree recursively.
 */
void xml_tree_delete(struct xml_tree *restrict victim)
    __attribute__ ((__nonnull__ (1)));


/*
 * return the first child with the name `name'.
 */
struct xml_tree* xml_lookup(const struct xml_tree *restrict tree, const char *restrict name)
    __attribute__ ((__nonnull__ (1, 2)));


/*
 * return the first child with the name `name', recursively.
 */
struct xml_tree* xml_reclookup(const struct xml_tree *restrict tree, unsigned int depth, ...)
    __attribute__ ((__nonnull__ (1)));


/*
 * return the first attr of given tree with the key `key'.
 */
char* xml_attrlookup(const struct xml_tree *restrict tree, const char *restrict key)
    __attribute__ ((__nonnull__ (1, 2)));

#endif /* !T_XML_H */

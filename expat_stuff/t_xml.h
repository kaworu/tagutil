#ifndef T_XML_H
#define T_XML_H

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
};

void xml_attr_delete(struct xml_attr *__restrict__ victim) __attribute__ ((__nonnull__ (1)));
void xml_tree_delete(struct xml_tree *__restrict__ victim) __attribute__ ((__nonnull__ (1)));
char* xml_push_data(struct xml_tree *__restrict__ elt, const char *data, size_t len)
    __attribute__ ((__nonnull__ (1, 2)));
struct xml_tree* xml_tree_new(struct xml_tree *__restrict__ parent, const char *__restrict__ name)
    __attribute__ ((__nonnull__ (2)));
struct xml_attr* xml_push_attr(struct xml_tree *__restrict__ elt, const char *__restrict__ key, const char *__restrict__ val)
    __attribute__ ((__nonnull__ (1, 2, 3)));


#endif /* !T_XML_H */

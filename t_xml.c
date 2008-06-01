/*
 * t_xml.c
 *
 * xml parser for tagutil.
 * use expat.
 */

#include <stdlib.h>
#include <expat.h>

#include "t_toolkit.h"
#include "t_xml.h"

static struct XML_ParserStruct *parser;

void xml_attr_delete(struct xml_attr *__restrict__ victim)
{

    assert(victim != NULL);

    free((void *)victim->key);
    free((void *)victim->val);
    free((void *)victim);
}


void xml_tree_delete(struct xml_tree *__restrict__ victim)
{
    int i;

    assert(victim != NULL);

    switch (victim->type) {
    case XML_LEAF:
        if (victim->content.data != NULL)
            free(victim->content.data);
        break;
    case XML_NODE:
        for (i = 0; i < victim->content_len; i++)
            xml_tree_delete(victim->content.childv[i]);

        if (victim->content.childv != NULL)
            free((void *)victim->content.childv);
        break;
    default:
        die(("hoho.. unknow xml_trtype.\n"));
    }

    for (i = 0; i < victim->attrc; i++)
        xml_attr_delete(victim->attrv[i]);

    if (victim->attrv != NULL)
        free((void *)victim->attrv);

    free((void *)victim->name);
    free((void *)victim);
}


/* NULL terminate ! u8 
 * TODO: parser can call more than 1 time this method (by handler) because it can stop
 * in the midle of reading data. then it will call at least 2 times, then we have to concat
 */
char *
xml_push_data(struct xml_tree *__restrict__ elt, const char *__restrict__ data, size_t len)
{

    assert(data != NULL);
    assert(elt  != NULL);
    assert(elt->type == XML_LEAF);

    elt->content.data =
        xrealloc(elt->content.data, elt->content_len + len + 1);

    strncpy(elt->content.data + elt->content_len, data, len);

    elt->content_len += len;

    return (elt->content.data);
}


struct xml_tree *
xml_tree_new(struct xml_tree *__restrict__ parent, const char *__restrict__ name)
{
    struct xml_tree *res;

    assert(name != NULL);
    assert(name[0] != '\0');

    res = xcalloc(1, sizeof(struct xml_tree));

    res->name = str_copy(name);
    res->type = XML_LEAF;
    res->parent = parent;

    if (parent != NULL) {
        res->depth = parent->depth + 1;

        if (parent->type == XML_LEAF) {
            assert(parent->content.data == NULL);
            parent->type = XML_NODE;
        }

        parent->content_len++;
        parent->content.childv = xrealloc(parent->content.childv,
                parent->content_len * sizeof(struct xml_tree *));
        parent->content.childv[parent->content_len - 1] = res;
    }

    return (res);
}

struct xml_attr *
xml_push_attr(struct xml_tree *__restrict__ elt, const char *__restrict__ key, const char *__restrict__ val)
{
    struct xml_attr *cur_attr;

    assert(elt != NULL);
    assert(key != NULL);
    assert(val != NULL);
    assert(key[0] != '\0');
    assert(val[0] != '\0');

    elt->attrc++;
    elt->attrv = xrealloc(elt->attrv, elt->attrc * sizeof(struct xml_attr *));
    cur_attr = elt->attrv[elt->attrc - 1] = xmalloc(sizeof(struct xml_attr));

    cur_attr->key = str_copy(key);
    cur_attr->val = str_copy(val);

    return (cur_attr);
}

static void XMLCALL
xml_start(void *userdata, const char *elt, const char **attr)
{
    struct xml_tree *tree;
    int i;

    assert(elt  != NULL);
    assert(attr != NULL);

    tree = xml_tree_new((struct xml_tree *)userdata, elt);

    for (i = 0; attr[i] != NULL; i += 2)
        xml_push_attr(tree, attr[i], attr[i + 1]);

    XML_SetUserData(parser, (void *)tree);
}

static void XMLCALL
xml_charhndl(void *userdata, const XML_Char *s, int len)
{
    xml_push_data((struct xml_tree *)userdata, (const char *)s, len);
}

static void XMLCALL
xml_end(void *userdata, const char *el)
{
    struct xml_tree *t;

    assert(el != NULL);
    assert(userdata != NULL);

    t = (struct xml_tree *)userdata;

    assert(strcmp(t->name, el) == 0);
    assert(t->parent != NULL);

    XML_SetUserData(parser, (void *)(t->parent));
}

int
main(int argc, char *argv[])
{
    int done;
    int len;
    char buf[BUFSIZ];

    parser = XML_ParserCreate((const XML_Char *)NULL);

    if (parser == NULL) {
        fprintf(stderr, "Couldn't allocate memory for parser\n");
        exit(-1);
    }

    XML_SetElementHandler(parser, xml_start, xml_end);
    XML_SetCharacterDataHandler(parser, xml_charhndl);
    XML_SetUserData(parser, xml_tree_new(NULL, "__root__"));

    for (;;) {
        len = (int) fread(buf, 1, BUFSIZ, stdin);
        if (ferror(stdin)) {
            fprintf(stderr, "Read error\n");
            exit(-1);
        }
        done = feof(stdin);

        if (XML_Parse(parser, buf, len, done) == XML_STATUS_ERROR) {
            fprintf(stderr, "Parse error at line %ul" "u:\n%s\n",
                    XML_GetCurrentLineNumber(parser),
                    XML_ErrorString(XML_GetErrorCode(parser)));
            exit(-1);
        }

        if (done)
            break;
    }
    printf("root_name: %s\n", ((struct xml_tree *)XML_GetUserData(parser))->name);
    xml_tree_delete((struct xml_tree *)XML_GetUserData(parser));
    XML_ParserFree(parser);
            return (0);
}

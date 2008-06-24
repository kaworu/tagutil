/*
 * t_xml.c
 *
 * xml parser for tagutil.
 * use expat.
 */
#include <stdarg.h>

#include <expat.h>

#include "config.h"
#include "t_xml.h"
#include "t_toolkit.h"


/*
 * xml_tree_delete() helper.
 */
static inline void xml_attr_delete(struct xml_attr *restrict victim)
    __attribute__ ((__nonnull__ (1)));


/* we need data to be NULL-terminate.
 * parser can call more than 1 time this method (by handler) because it can stop
 * in the midle of reading data.
 *
 * called by expat handlers.
 */
static char* xml_push_data(struct xml_tree *restrict elt, const char *data, size_t len)
    __attribute__ ((__nonnull__ (1, 2)));


/*
 * add attribute to given xml_tree.
 *
 * called by expat handlers.
 */
static struct xml_attr* xml_push_attr(struct xml_tree *restrict elt, const char *restrict key, const char *restrict val)
    __attribute__ ((__nonnull__ (1, 2, 3)));


/* expat handlers */
static void XMLCALL xml_start(void *userdata, const char *elt, const char **attr);
static void XMLCALL xml_charhndl(void *userdata, const XML_Char *s, int len);
static void XMLCALL xml_end(void *userdata, const char *el);
/* expat parser. */
static struct XML_ParserStruct *parser;


struct xml_tree *
xml_parse(const char *restrict data)
{
    struct xml_tree *res;

    assert_not_null(data);

    parser = XML_ParserCreate(NULL);

    if (parser == NULL)
        err(ENOMEM, "Couldn't allocate memory for parser\n");

    /* link handler functions to parser */
    XML_SetStartElementHandler(parser, xml_start);
    XML_SetEndElementHandler(parser, xml_end);
    XML_SetCharacterDataHandler(parser, xml_charhndl);
    XML_SetUserData(parser, xml_tree_new(NULL, "__root__"));

    /* do the parsing */
    if (XML_Parse(parser, data, strlen(data), true) == XML_STATUS_ERROR)
        errx(errno, "Parse error at line %lu: %s",
                    XML_GetCurrentLineNumber(parser),
                    XML_ErrorString(XML_GetErrorCode(parser)));

    res = XML_GetUserData(parser);
    /* we don't need it anymore. It *won't* free the result */
    XML_ParserFree(parser);

    return (res);
}


void
xml_tree_show(const struct xml_tree *restrict tree)
{
    int i;

    if (tree == NULL)
        return;

    for (i = 0; i < tree->depth; i++)
        (void)printf("  ");

    if (tree->type == XML_LEAF)
        (void)printf("LEAF");
    else
        (void)printf("NODE");

    (void)printf("(%s):", tree->name);

    for (i = 0; i < tree->attrc; i++)
        (void)printf(" attr(%s,%s)",
                tree->attrv[i]->key, tree->attrv[i]->val);

    (void)printf(" {\n");
    switch(tree->type) {
    case XML_LEAF:
        if (tree->content.data != NULL) {
            for (i = 0; i < tree->depth + 1; i++)
                (void)printf("  ");
            (void)printf("%s\n", tree->content.data);
        }
        break;
    case XML_NODE:
        for (i = 0; i < tree->content_len; i++)
            xml_tree_show(tree->content.childv[i]);
        break;
    default:
        errx(-1, "bad xml_trtype in %s at %u", __FILE__, __LINE__);
        /* NOTREACHED */
    }

    for (i = 0; i < tree->depth; i++)
        (void)printf("  ");
    (void)printf("}\n");
}


void
xml_tree_delete(struct xml_tree *restrict victim)
{
    int i;

    assert_not_null(victim);

    switch (victim->type) {
    case XML_LEAF:
        if (victim->content.data != NULL)
            free(victim->content.data);
        break;
    case XML_NODE:
        for (i = 0; i < victim->content_len; i++)
            xml_tree_delete(victim->content.childv[i]);

        if (victim->content.childv != NULL)
            free(victim->content.childv);
        break;
    default:
        errx(-1, "bad xml_trtype in %s at %u", __FILE__, __LINE__);
        /* NOTREACHED */
    }

    for (i = 0; i < victim->attrc; i++)
        xml_attr_delete(victim->attrv[i]);

    if (victim->attrv != NULL)
        free(victim->attrv);

    free(victim->name);
    free(victim);
}


struct xml_tree *
xml_lookup(const struct xml_tree *restrict tree, const char *restrict name)
{
    int i;

    assert_not_null(tree);
    assert_not_null(name);

    if (tree->type != XML_NODE)
        return (NULL);

    for (i = 0; i < tree->content_len; i++) {
        if (strcmp(tree->content.childv[i]->name, name) == 0)
            return (tree->content.childv[i]);
    }

    return (NULL);
}


struct xml_tree *
xml_reclookup(const struct xml_tree *restrict tree, unsigned int depth, ...)
{
    va_list ap;
    struct xml_tree *current;

    assert_not_null(tree);

    va_start(ap, depth);
    current = NULL;

    while (depth--) {
        current = xml_lookup(current == NULL ? tree : current, va_arg(ap, const char *));
        if (current == NULL)
            return (NULL);
    }

    va_end(ap);
    return (current);
}


char *
xml_attrlookup(const struct xml_tree *restrict tree, const char *restrict key)
{
    int i;

    assert_not_null(tree);
    assert_not_null(key);

    for (i = 0; i < tree->attrc; i++) {
        if (strcmp(tree->attrv[i]->key, key) == 0)
            return (tree->attrv[i]->val);
    }

    return (NULL);
}


static inline void
xml_attr_delete(struct xml_attr *restrict victim)
{

    assert_not_null(victim);

    free(victim->key);
    free(victim->val);
    free(victim);
}


static char *
xml_push_data(struct xml_tree *restrict elt, const char *restrict data, size_t len)
{

    assert_not_null(data);
    assert_not_null(elt);
    assert(elt->type == XML_LEAF);
    assert(len > 0);
#if !defined(NDEBUG)
    assert(!elt->frozen);
#endif

    /* our parser don't care about blank lines and indent. */
    if (empty_line(data))
        return (NULL);

    elt->content.data =
        xrealloc(elt->content.data, elt->content_len + len + 1);

    strncpy(elt->content.data + elt->content_len, data, len + 1);
    elt->content_len += len;

    return (elt->content.data);
}


struct xml_tree *
xml_tree_new(struct xml_tree *restrict parent, const char *restrict name)
{
    struct xml_tree *res;

    assert_not_null(name);
    assert(name[0] != '\0');

    res = xcalloc(1, sizeof(struct xml_tree));

    res->name = xstrdup(name);
    res->type = XML_LEAF;
    res->parent = parent;
#if !defined(NDEBUG)
    res->frozen = false;
#endif

    if (parent != NULL) {
        res->depth = parent->depth + 1;

        if (parent->type == XML_LEAF) {
            assert_null(parent->content.data);
            parent->type = XML_NODE;
        }

        parent->content_len++;
        parent->content.childv = xrealloc(parent->content.childv,
                parent->content_len * sizeof(struct xml_tree *));
        parent->content.childv[parent->content_len - 1] = res;
    }

    return (res);
}


static struct xml_attr *
xml_push_attr(struct xml_tree *restrict elt, const char *restrict key, const char *restrict val)
{
    struct xml_attr *cur_attr;

    assert_not_null(elt);
    assert_not_null(key);
    assert_not_null(val);
    assert(key[0] != '\0');
    assert(val[0] != '\0');
#if !defined(NDEBUG)
    assert(!elt->frozen);
#endif

    elt->attrc++;
    elt->attrv = xrealloc(elt->attrv, elt->attrc * sizeof(struct xml_attr *));
    cur_attr = elt->attrv[elt->attrc - 1] = xmalloc(sizeof(struct xml_attr));

    cur_attr->key = xstrdup(key);
    cur_attr->val = xstrdup(val);

    return (cur_attr);
}

static void XMLCALL
xml_start(void *userdata, const char *elt, const char **attr)
{
    struct xml_tree *tree;
    int i;

    assert_not_null(elt);
    assert_not_null(attr);

    tree = xml_tree_new((struct xml_tree *)userdata, elt);

    for (i = 0; attr[i] != NULL; i += 2)
        xml_push_attr(tree, attr[i], attr[i + 1]);

    XML_SetUserData(parser, tree);
}

static void XMLCALL
xml_charhndl(void *userdata, const XML_Char *s, int len)
{
    char *str;
    str = xcalloc(len + 1, sizeof(char));
    strncpy(str, s, len);

    xml_push_data((struct xml_tree *)userdata, str, len);

    free(str);
}

static void XMLCALL
xml_end(void *userdata, const char *el)
{
    struct xml_tree *tree;

    assert_not_null(el);
    assert_not_null(userdata);

    tree = (struct xml_tree *)userdata;

    assert(strcmp(tree->name, el) == 0);
    assert_not_null(tree->parent);
#if !defined(NDEBUG)
    assert(!tree->frozen);
    tree->frozen = true;
#endif

    XML_SetUserData(parser, tree->parent);
}


#if 0
/*
 * loosely main method that parse stdin.
 */
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

    XML_SetStartElementHandler(parser, xml_start);
    XML_SetEndElementHandler(parser, xml_end);
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
            fprintf(stderr, "Parse error at line %lu" "u:\n%s\n",
                    XML_GetCurrentLineNumber(parser),
                    XML_ErrorString(XML_GetErrorCode(parser)));
            exit(-1);
        }

        if (done)
            break;
    }

    printf("root_name: %s\n", ((struct xml_tree *)XML_GetUserData(parser))->name);
    xml_tree_show((struct xml_tree *)XML_GetUserData(parser));
    xml_tree_delete((struct xml_tree *)XML_GetUserData(parser));
    XML_ParserFree(parser);
            return (0);
}
#endif

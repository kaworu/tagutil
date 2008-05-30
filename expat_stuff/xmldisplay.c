#include <stdlib.h>
#include <stdio.h>
#include <expat.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#if 1
static struct XML_ParserStruct *parser;
static int alc_c, alc_b;
static int attrcounter, datacounter, treecounter;
#define INCR_ALLOC_COUNTER(x) alc_c++; alc_b += (x);
#define die(x) exit(1)
#define D(x) x
static __inline__ void *
xmalloc(const size_t size)
{
    void *ptr;

    INCR_ALLOC_COUNTER(size);

    ptr = malloc(size);
    if (size > 0 && ptr == NULL)
        die(("malloc failed.\n"));

    return (ptr);
}


static __inline__ void *
xcalloc(const size_t nmemb, const size_t size)
{
    void *ptr;

    INCR_ALLOC_COUNTER(nmemb * size);

    ptr = calloc(nmemb, size);
    if (ptr == NULL && size > 0 && nmemb > 0)
        die(("calloc failed.\n"));

    return (ptr);
}


static __inline__ void *
xrealloc(void *old_ptr, const size_t new_size)
{
    void *ptr;

    INCR_ALLOC_COUNTER(new_size);

    ptr = realloc(old_ptr, new_size);
    if (ptr == NULL && new_size > 0)
        die(("realloc failed.\n"));

    return (ptr);
}


static __inline__ char *
str_copy(const char *__restrict__ src)
{
    char *result;
    size_t len;

    assert(src != NULL);

    len = strlen(src);
    result = xcalloc(len + 1, sizeof(char));
    strncpy(result, src, len);

    return (result);
}
#endif

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
        die(NULL);
    }

    for (i = 0; i < victim->attrc; i++) {
        free((void *)victim->attrv[i]->key);
        free((void *)victim->attrv[i]->val);
        free((void *)victim->attrv[i]);
    }
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

    D(datacounter++);
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

    D(treecounter++);
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

    D(attrcounter++);
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

#if 0

ELEMENT(metadata); ATTR(xmlns,'http://musicbrainz.org/ns/mmd-1.0#'),ATTR(xmlns:ext,'http://musicbrainz.org/ns/ext-1.0#'),
  ELEMENT(track-list); ATTR(count,'5'),ATTR(offset,'0'),
    ELEMENT(track); ATTR(id,'0476dc62-2b58-43fe-96b3-49b76ba17a08'),ATTR(ext:score,'99'),
      ELEMENT(title); 
        DATA(On My Shoulders);
      END_OF_ELEMENT(title); 
      ELEMENT(duration); 
        DATA(318306);
      END_OF_ELEMENT(duration); 
      ELEMENT(artist); ATTR(id,'c3477250-bc5f-44e9-aa0a-144dd2d7a935'),
        ELEMENT(name); 
          DATA(The Dø);
        END_OF_ELEMENT(name); 
      END_OF_ELEMENT(artist); 
      ELEMENT(release-list); 
        ELEMENT(release); ATTR(id,'3a871cef-e297-486a-b355-297e7b44e1fb'),
          ELEMENT(title); 
            DATA(Fair 2008);
          END_OF_ELEMENT(title); 
          ELEMENT(track-list); ATTR(offset,'12'),ATTR(count,'15'),
          END_OF_ELEMENT(track-list); 
        END_OF_ELEMENT(release); 
      END_OF_ELEMENT(release-list); 
    END_OF_ELEMENT(track);
#endif

int
main(int argc, char *argv[])
{
    int done;
    int len;
    char buf[BUFSIZ];

    D(attrcounter = datacounter = treecounter =)
    alc_c = alc_b = 0;
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
    printf("alc_c = %d, alc_b = %d\n", alc_c, alc_b);
    D(printf("datacounter = %d, treecounter = %d, attrcounter = %d\n",
                datacounter, treecounter, attrcounter));
    XML_ParserFree(parser);
            return (0);
}

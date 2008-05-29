#include <stdlib.h>
#include <stdio.h>
#include <expat.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

struct xml_attr {
    char *key, *val;
};
typedef enum { XML_LEAF, XML_NODE } xml_trtype;

struct xml_tree {
    char *name;
    xml_trtype type;
    int attrc, childc;
    struct xml_attr *attrv;
    union {
        char *data;                 /* <==> type == XML_LEAF */
        struct xml_tree *childv; /* <==> type == XML_NODE */ 
    } inner;
};

static __inline__ int xml_set_name(struct xml_tree *__restrict__ elt, const char *__restrict__ name)
    __attribute__ ((__nonnull__ (1, 2)));
static __inline__ struct xml_attr* xml_push_attr(struct xml_tree *__restrict__ elt, const char *__restrict__ key, const char *__restrict__ val)
    __attribute__ ((__nonnull__ (1, 2, 3)));

static __inline__ int
xml_set_name(struct xml_tree *__restrict__ elt, const char *__restrict__ src)
{
    size_t len;

    assert(elt != NULL);
    assert(src != NULL);

    len = strlen(src);
    elt->name = xcalloc(len + 1, sizeof(char));
    strncpy(elt->name, src, len);

    return (len);
}
static __inline__ struct xml_attr *
xml_push_attr(struct xml_tree *__restrict__ elt, const char *__restrict__ key, const char *__restrict__ val)
{
    struct xml_attr *cur_attr;
    assert(elt != NULL);
    assert(key != NULL);
    assert(val != NULL);

    elt->attrv[elt->attrc] = xmalloc(sizeof(struct xml_attr));
    cur_attr = elt->attrv[elt->attrc++];
}
static void XMLCALL
end(void *userdata, const char *el)
{
    int i;
    struct mb_data *d;
    d = (struct mb_data *)userdata;
    d->Depth--;
}

static void XMLCALL
charhndl(void *userdata, const XML_Char *s, int len)
{
    struct mb_data *d;
    char *buf;
    int i;
    d = (struct mb_data *)userdata;
    /* NULL-terminate s plz */
}

static void XMLCALL
start(void *userdata, const char *elt, const char **attr)
{
    int i;
    struct mb_data *d;
    data = (struct mb_data *)userdata;

    data->Depth++;
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

static void XMLCALL start(void *userdata, const char *el, const char **attr);
static void XMLCALL end(void *userdata, const char *el);
static void XMLCALL charhndl(void *userdata, const XML_Char *s, int len);

int
main(int argc, char *argv[])
{
    int done;
    int len;
    struct XML_ParserStruct *p;
    struct mb_data d;
    char buf[BUFSIZ];


    d.Depth = 0;

    p = XML_ParserCreate((const XML_Char *)NULL);
    if (p == NULL) {
        fprintf(stderr, "Couldn't allocate memory for parser\n");
        exit(-1);
    }

    XML_SetElementHandler(p, start, end);
    XML_SetCharacterDataHandler(p, charhndl);
    XML_SetUserData(p, (void *)&d);

    for (;;) {
        len = (int) fread(buf, 1, BUFSIZ, stdin);
        if (ferror(stdin)) {
            fprintf(stderr, "Read error\n");
            exit(-1);
        }
        done = feof(stdin);

        if (XML_Parse(p, buf, len, done) == XML_STATUS_ERROR) {
            fprintf(stderr, "Parse error at line %ul" "u:\n%s\n",
                    XML_GetCurrentLineNumber(p),
                    XML_ErrorString(XML_GetErrorCode(p)));
            exit(-1);
        }

        if (done)
            break;
    }
    XML_ParserFree(p);
    return 0;
}

/*
 * t_lexer.c
 *
 * a hand writted lexer for tagutil.
 * used by the filter function.
 */

#include <stdlib.h>
#include <strings.h> /* strncasecmp(3) */

#include "config.h"
#include "t_lexer.h"
#include "t_toolkit.h"


struct lexer *
new_lexer(const char *restrict str)
{
    struct lexer *L;

    assert_not_null(str);

    L = xmalloc(sizeof(struct lexer));

    L->source = str;
    L->index = 0;
    L->current.kind = TSTART;

    return (L);
}


void
lex(struct lexer *restrict L)
{
    const char *source;
    int *Lindex;
    struct token *current;
    /* String */
    size_t skip;
    /* keywords */
    bool found;
    size_t i;

    assert_not_null(L);

    source   = L->source;
    Lindex   = &L->index;
    current  = &L->current;

    /* eat blank chars */
    while (source[*Lindex] != '\0' && is_blank(source[*Lindex]))
        *Lindex += 1;

    switch(source[*Lindex]) {
    case '\0':
        current->kind = TEOS;
        current->start = current->end = *Lindex;
        break;
    case '!':
        if (source[*Lindex + 1] == '=') {
            current->kind  = TDIFF;
            current->start = *Lindex;
            current->end   = *Lindex + 1;
            *Lindex += 2;
        }
        else {
            current->kind = TNOT;
            current->start = current->end = *Lindex;
            *Lindex += 1;
        }
        break;
    case '=':
        switch (source[*Lindex + 1]) {
        case '=':
            current->kind = TEQ;
            current->start = *Lindex;
            current->end = *Lindex + 1;
            *Lindex += 2;
            break;
        case '~':
            current->kind = TMATCH;
            current->start = *Lindex;
            current->end = *Lindex + 1;
            *Lindex += 2;
            break;
        default:
            errx(-1, "lexer error at %d: got '%c', expected '=' for EQ or '~' for MATCH",
                    *Lindex, source[*Lindex + 1]);
            /* NOT REACHED */
        }
        break;
    case '|':
        if (source[*Lindex + 1] == '|') {
            current->kind = TOR;
            current->start = *Lindex;
            current->end = *Lindex + 1;
            *Lindex += 2;
        }
        else
            errx(-1, "lexer error at %d: got '%c', expected '|' for OR",
                    *Lindex, source[*Lindex + 1]);
        break;
    case '&':
        if (source[*Lindex + 1] == '&') {
            current->kind = TAND;
            current->start = *Lindex;
            current->end = *Lindex + 1;
            *Lindex += 2;
        }
        else
            errx(-1, "lexer error at %d: got '%c', expected '&' for AND",
                    *Lindex, source[*Lindex + 1]);
        break;
    case '<':
        if (source[*Lindex + 1] == '=') {
            current->kind = TLE;
            current->start = *Lindex;
            current->end = *Lindex + 1;
            *Lindex += 2;
        }
        else {
            current->kind = TLT;
            current->start = current->end = *Lindex;
            *Lindex += 1;
        }
        break;
    case '>':
        if (source[*Lindex + 1] == '=') {
            current->kind = TGE;
            current->start = *Lindex;
            current->end = *Lindex + 1;
            *Lindex += 2;
        }
        else {
            current->kind = TGT;
            current->start = current->end = *Lindex;
            *Lindex += 1;
        }
        break;
    case '(':
        current->kind = TOPAREN;
        current->start = current->end = *Lindex;
        *Lindex += 1;
        break;
    case ')':
        current->kind = TCPAREN;
        current->start = current->end = *Lindex;
        *Lindex += 1;
        break;
    case '0': /* FALLTHROUGH */
    case '1': /* FALLTHROUGH */
    case '2': /* FALLTHROUGH */
    case '3': /* FALLTHROUGH */
    case '4': /* FALLTHROUGH */
    case '5': /* FALLTHROUGH */
    case '6': /* FALLTHROUGH */
    case '7': /* FALLTHROUGH */
    case '8': /* FALLTHROUGH */
    case '9':
        current->kind = TINT;
        current->value.integer = 0;
        current->start = *Lindex;
        while (is_number(source[*Lindex])) {
            /* mean that 00000012 == 12 */
            current->value.integer *= 10;
            current->value.integer += source[*Lindex] - '0';
            *Lindex += 1;
        }
        current->end = *Lindex - 1;
        break;
    case '"':
        skip = 0;
        current->kind = TSTRING;
        current->start = *Lindex;
        *Lindex += 1; /* skip " */
        while (source[*Lindex] != '\0' && source[*Lindex] != '"') {
            if (source[*Lindex] == '\\') {
                *Lindex += 2;
                skip += 1;
            }
            else
                *Lindex += 1;
        }

        if (source[*Lindex] == '"')
            current->end = *Lindex;
        else {
            errx(-1, "lexer error at %d: got '%c', expected '\"' for STRING, string %s?",
                    *Lindex, source[*Lindex], source[*Lindex] == '\0' ? "not ended" : "too long");
        }

        current->alloclen = current->end - current->start - skip;
        current->value.string = xcalloc(current->alloclen, sizeof(char));
        *Lindex = current->start + 1;
        skip = 0;

        while (source[*Lindex] != '"') {
            if (source[*Lindex] == '\\') {
                *Lindex += 1;
                skip += 1;
            }
            current->value.string[*Lindex - skip - current->start - 1] = source[*Lindex];
            *Lindex += 1;
        }

        current->value.string[current->alloclen - 1] = '\0';
        *Lindex += 1; /* skip " */
        break;
    default:
        /* keyword, not optimized at all, no gperf or so */
        current->start = *Lindex;
        found = false;
        for (i = 0; !found && i < len(keywords); i++) {
            if (strncasecmp(&source[*Lindex], keywords[i].lexem, keywords[i].lexemlen) == 0) {
                current->kind = keywords[i].kind;
                *Lindex += keywords[i].lexemlen;
                current->end = *Lindex - 1;
                found = true;
            }
        }

        if (!found)
            errx(-1, "lexer error at %d: got '%c'..., expected a keyword", *Lindex, source[*Lindex]);
        break;
    }
}

#if 0
int
main(int argc, char *argv[])
{
    struct lexer *L;
    int the_end;

    if (argc < 2)
        errx(-1, "usage: %s [string]\n", argv[0]);

    the_end = 0;
    L = new_lexer(argv[1]);

    while (!the_end) {
        (void)printf("%s", token_to_s(L->current.kind));

        switch (L->current.kind) {
            case TINT:
                (void)printf("(%d) ", L->current.value.integer);
                break;
            case TSTRING:
                (void)printf("(%s)[%d] ", L->current.value.string, (int)L->current.alloclen);
                break;
            case TEOS:
                (void)printf("\n");
                the_end = 1;
                break;
            default:
                (void)printf(" ");
                break;
        }
        lex(L);
    }

    return (0);
}
#endif

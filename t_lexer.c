/*
 * t_lexer.c
 *
 * a hand writted lexer for tagutil.
 * used by the filter function.
 */
#include "t_config.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h> /* strncasecmp(3) */

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
    const char *Lsource;
    int *Lindex;
    struct token *Lcurrent;
    /* String & Regexp */
    size_t skip;
    char c;
    char *s, *errbuf;
    int error;
    bool iflag, mflag;
    /* keywords */
    bool found;
    size_t i;

    assert_not_null(L);

    Lsource   = L->source;
    Lindex   = &L->index;
    Lcurrent  = &L->current;

    /* eat blank chars */
    while (Lsource[*Lindex] != '\0' && is_blank(Lsource[*Lindex]))
        *Lindex += 1;

    switch(Lsource[*Lindex]) {
    case '\0':
        Lcurrent->kind = TEOS;
        Lcurrent->start = Lcurrent->end = *Lindex;
        break;
    case '!':
        if (Lsource[*Lindex + 1] == '=') {
            Lcurrent->kind  = TDIFF;
            Lcurrent->start = *Lindex;
            Lcurrent->end   = *Lindex + 1;
            *Lindex += 2;
        }
        else {
            Lcurrent->kind = TNOT;
            Lcurrent->start = Lcurrent->end = *Lindex;
            *Lindex += 1;
        }
        break;
    case '=':
        switch (Lsource[*Lindex + 1]) {
        case '=':
            Lcurrent->kind = TEQ;
            Lcurrent->start = *Lindex;
            Lcurrent->end = *Lindex + 1;
            *Lindex += 2;
            break;
        case '~':
            Lcurrent->kind = TMATCH;
            Lcurrent->start = *Lindex;
            Lcurrent->end = *Lindex + 1;
            *Lindex += 2;
            break;
        default:
            errx(-1, "lexer error at %d: got '%c', expected '=' for EQ or '~' for MATCH",
                    *Lindex, Lsource[*Lindex + 1]);
            /* NOT REACHED */
        }
        break;
    case '|':
        if (Lsource[*Lindex + 1] == '|') {
            Lcurrent->kind = TOR;
            Lcurrent->start = *Lindex;
            Lcurrent->end = *Lindex + 1;
            *Lindex += 2;
        }
        else
            errx(-1, "lexer error at %d: got '%c', expected '|' for OR",
                    *Lindex, Lsource[*Lindex + 1]);
        break;
    case '&':
        if (Lsource[*Lindex + 1] == '&') {
            Lcurrent->kind = TAND;
            Lcurrent->start = *Lindex;
            Lcurrent->end = *Lindex + 1;
            *Lindex += 2;
        }
        else
            errx(-1, "lexer error at %d: got '%c', expected '&' for AND",
                    *Lindex, Lsource[*Lindex + 1]);
        break;
    case '<':
        if (Lsource[*Lindex + 1] == '=') {
            Lcurrent->kind = TLE;
            Lcurrent->start = *Lindex;
            Lcurrent->end = *Lindex + 1;
            *Lindex += 2;
        }
        else {
            Lcurrent->kind = TLT;
            Lcurrent->start = Lcurrent->end = *Lindex;
            *Lindex += 1;
        }
        break;
    case '>':
        if (Lsource[*Lindex + 1] == '=') {
            Lcurrent->kind = TGE;
            Lcurrent->start = *Lindex;
            Lcurrent->end = *Lindex + 1;
            *Lindex += 2;
        }
        else {
            Lcurrent->kind = TGT;
            Lcurrent->start = Lcurrent->end = *Lindex;
            *Lindex += 1;
        }
        break;
    case '(':
        Lcurrent->kind = TOPAREN;
        Lcurrent->start = Lcurrent->end = *Lindex;
        *Lindex += 1;
        break;
    case ')':
        Lcurrent->kind = TCPAREN;
        Lcurrent->start = Lcurrent->end = *Lindex;
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
        Lcurrent->kind = TINT;
        Lcurrent->value.integer = 0;
        Lcurrent->start = *Lindex;
        while (is_digit(Lsource[*Lindex])) {
            Lcurrent->value.integer *= 10;
            Lcurrent->value.integer += Lsource[*Lindex] - '0';
            *Lindex += 1;
        }
        Lcurrent->end = *Lindex - 1;
        break;
    case '/': /* FALLTHROUGH */
    case '"':
        c = Lsource[*Lindex];
        skip = 0;
        Lcurrent->kind = (c == '"' ? TSTRING : TREGEX);
        Lcurrent->start = *Lindex;
        *Lindex += 1;
        while (Lsource[*Lindex] != '\0' && Lsource[*Lindex] != c) {
            if (Lsource[*Lindex] == '\\') {
                *Lindex += 2;
                skip += 1;
            }
            else
                *Lindex += 1;
        }

        if (Lsource[*Lindex] == c)
            Lcurrent->end = *Lindex;
        else {
            errx(-1, "lexer error at %d: got '%c', expected '%c' for %s, not ended?",
                    *Lindex, Lsource[*Lindex], c,
                    Lcurrent->kind == TSTRING ? "STRING" : "REGEX");
        }

        /* do the copy */
        Lcurrent->alloclen = Lcurrent->end - Lcurrent->start - skip;
        Lcurrent->value.string = xcalloc(Lcurrent->alloclen, sizeof(char));
        *Lindex = Lcurrent->start + 1;
        skip = 0;
        while (Lsource[*Lindex] != c) {
            if (Lsource[*Lindex] == '\\') {
                *Lindex += 1;
                skip += 1;
            }
            Lcurrent->value.string[*Lindex - skip - Lcurrent->start - 1] = Lsource[*Lindex];
            *Lindex += 1;
        }

        Lcurrent->value.string[Lcurrent->alloclen - 1] = '\0';
        *Lindex += 1;

        /* handle regex options */
        if (Lcurrent->kind == TREGEX) {
            iflag = mflag = false;

            while (Lsource[*Lindex] != '\0' && strchr("im", Lsource[*Lindex]) != NULL) {
                switch (Lsource[*Lindex]) {
                case 'i':
                    if (iflag)
                        errx(-1, "lexer error at %d: option %c given twice", *Lindex, Lsource[*Lindex]);
                    iflag = true;
                    break;
                case 'm':
                    if (mflag)
                        errx(-1, "lexer error at %d: option %c given twice", *Lindex, Lsource[*Lindex]);
                    mflag = true;
                    break;
                default:
                    errx(-1, "lexer nasty bug.");
                    /* NOTREACHED */
                }
                *Lindex += 1;
            }
            Lcurrent->end = *Lindex - 1;
            s = Lcurrent->value.string;
            error = regcomp(&Lcurrent->value.regex, s,
                    REG_NOSUB | REG_EXTENDED  |
                    (iflag ? REG_ICASE   : 0) |
                    (mflag ? REG_NEWLINE : 0) );

            if (error != 0) {
                errbuf = xcalloc(BUFSIZ, sizeof(char));
                (void)regerror(error, &Lcurrent->value.regex, errbuf, BUFSIZ);
                errx(-1, "lexer error, can't compile regex '%s': %s", s, errbuf);
            }
            xfree(s); /* free the regex string, no more needed */
        }
        break;
    default:
        /* keyword, not optimized at all, no gperf or so */
        Lcurrent->start = *Lindex;
        found = false;
        for (i = 0; !found && i < len(keywords); i++) {
            if (strncasecmp(&Lsource[*Lindex], keywords[i].lexem, keywords[i].lexemlen) == 0) {
                Lcurrent->kind = keywords[i].kind;
                *Lindex += keywords[i].lexemlen;
                Lcurrent->end = *Lindex - 1;
                found = true;
            }
        }

        if (!found)
            errx(-1, "lexer error at %d: got '%c'..., expected a keyword", *Lindex, Lsource[*Lindex]);
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

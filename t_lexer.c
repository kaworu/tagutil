/*
 * t_lexer.c
 *
 * a hand writted lexer for tagutil.
 * used by the filter function.
 */
#include "t_config.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "t_lexer.h"
#include "t_toolkit.h"


/*
 * put next char in L->c and increment L->cindex.
 */
_t__nonnull(1)
static inline char lexc(struct lexer *restrict L);



struct lexer *
new_lexer(const char *restrict source)
{
    char *s;
    size_t len;
    struct lexer *L;

    assert_not_null(source);

    len = strlen(source) + 1;
    L = xmalloc(sizeof(struct lexer) + len);

    L->srclen = len - 1;
    s = (char *)(L + 1);
    strlcpy(s, source, len);
    L->source = s;
    L->c = -1;
    L->cindex = -1;

    return (L);
}


static inline char
lexc(struct lexer *restrict L)
{

    assert_not_null(L);

    if (L->c != '\0') {
    /* move forward of one char */
        assert(L->cindex < (long)L->srclen);
        L->cindex += 1;
        L->c = L->source[L->cindex];
    }
    else
        assert(L->cindex > 0 && (size_t)L->cindex == L->srclen);

    return (L->c);
}


void
lex_number(struct lexer *restrict L, struct token *restrict t)
{
    char *num, *endptr; /* new buffer pointers */
    const char *start, *end; /* source pointers */
    bool isfp;

    assert_not_null(L);
    assert_not_null(t);
    assert(isnumber(L->c) || L->c == '.' || L->c == '+' || L->c == '-');

    start = L->source + L->cindex;
    if (L->c == '+' || L->c == '-')
        (void)lexc(L);

    if (L->c == '0' && lexc(L) != '.') {
    /* int 0 */
        t->kind = TINT;
        t->value.integer = 0;
    }
    else {
        isfp = false;
        while (isdigit(L->c) || L->c == '.') {
            if (L->c == '.') {
                if (isfp)
                    break;
                else
                    isfp = true;
            }
            (void)lexc(L);
        }
        end = L->source + L->cindex - 1;
        assert(end >= start);
        num = xcalloc((end - start + 1) + 1, sizeof(char));
        memcpy(num, start, end - start + 1);
        if (isfp) {
            t->kind = TDOUBLE;
            t->value.dbl = strtod(num, &endptr);
            if (!strempty(endptr))
                errx(-1, "bad fp value"); /* FIXME beautifulize */
        }
        else {
            t->kind = TINT;
            t->value.integer = (int)strtol(num, &endptr, 10);
            if (!strempty(endptr))
                errx(-1, "bad int value"); /* FIXME beautifulize */
        }
        xfree(num);
    }
    t->end = L->cindex - 1;
}


void
lex_strlit_or_regex(struct lexer *restrict L, struct token  **tptr)
{
    struct token *t;
    char limit;
    unsigned int skip, i;
    /* used for regexp */
    bool iflag, mflag;
    int error;
    char *s, *errbuf;

    assert_not_null(L);
    assert_not_null(tptr);
    assert_not_null(*tptr);
    assert(L->c == '/' || L->c == '"');

    t = *tptr;
    skip = 0;
    limit = L->c;
    t->kind = (limit == '"' ? TSTRING : TREGEX);
    while (L->c != '\0' && lexc(L) != limit) {
        if (L->c == '\\' && lexc(L) == limit)
            skip += 1;
    }

    t->end = L->cindex;
    if (L->c != limit) {
        lex_error(L, t->start, t->end, "lexer error: unbalanced %c for %s",
                limit, limit == '"' ? "STRING" : "REGEX");
        /* NOTREACHED */
    }

    /* do the copy */
    t->alloclen  = t->end - t->start - skip;
    t = xrealloc(t, sizeof(struct token) + t->alloclen);
    *tptr = t;
    t->value.str = (char *)(t + 1);
    L->cindex = t->start;
    i = 0;
    while (lexc(L) != limit) {
        if (L->c == '\\') {
            if (lexc(L) != limit) {
            /* rewind */
                L->cindex -= 2;
                L->c = '\\';
            }
        }
        t->value.str[i++] = L->c;
    }
    t->value.str[i] = '\0';
    assert(strlen(t->value.str) + 1 == t->alloclen);

    /* skip limit */
    (void)lexc(L);

    /* handle regex options */
    if (t->kind == TREGEX) {
        iflag = mflag = false;

        while (L->c != '\0' && strchr("im", L->c) != NULL) {
            switch (L->c) {
                case 'i':
                    if (iflag)
                        goto regopt_error;
                    iflag = true;
                    break;
                case 'm':
                    if (mflag)
                        goto regopt_error;
                    mflag = true;
                    break;
                default:
                    assert(!("bug in regex option lexing"));
regopt_error:
                    lex_error(L, t->start, L->cindex,
                            "lexer error: option %c given twice for REGEX", L->c);
                    /* NOTREACHED */
            }
            (void)lexc(L);
        }
        t->end = L->cindex - 1;
        s = t->value.str;
        error = regcomp(&t->value.regex, s,
                REG_NOSUB | REG_EXTENDED  |
                (iflag ? REG_ICASE   : 0) |
                (mflag ? REG_NEWLINE : 0) );

        if (error != 0) {
            errbuf = xcalloc(BUFSIZ, sizeof(char));
            (void)regerror(error, &t->value.regex, errbuf, BUFSIZ);
            lex_error(L, t->start, t->end,
                    "lexer error: can't compile REGEX /%s/: %s", s, errbuf);
            /* NOTREACHED */
        }
    }
}


void
lex_tagkey(struct lexer *restrict L, struct token **tptr)
{
    struct token *t;
    unsigned int skip, i;

    assert_not_null(L);
    assert_not_null(tptr);
    assert_not_null(*tptr);
    assert(L->c == '%');

    t = *tptr;
    t->kind = TKEYWORD;
    if (lexc(L) == '{') {
    /* %{tag} */
        skip = 0;
        while (L->c != '\0' && lexc(L) != '}') {
            if (L->c == '\\') {
                if (lexc(L) == '}')
                    skip += 1;
            }
        }
        t->end = L->cindex;
        if (L->c != '}') {
            lex_error(L, t->start, t->end, "lexer error: unbalanced { for TKEYWORD");
            /* NOTREACHED */
        }

        /* do the copy */
        t->alloclen = t->end - t->start - 1 - skip;
        t = xrealloc(t, sizeof(struct token) + t->alloclen);
        *tptr = t;
        t->value.str = (char *)(t + 1);
        L->cindex    = t->start + 1;
        i = 0;
        while (lexc(L) != '}') {
            if (L->c == '\\') {
                if (lexc(L) != '}') {
                    /* rewind */
                    L->cindex -= 2;
                    L->c = '\\';
                }
            }
            t->value.str[i++] = L->c;
        }
        t->value.str[i] = '\0';
        assert(strlen(t->value.str) + 1 == t->alloclen);
        (void)lexc(L);
    }
    else {
    /* %tag */
        while (isalnum(L->c) || L->c == '-' || L->c == '_')
            lexc(L);
        t->end = L->cindex - 1;
        if (t->end == t->start) {
            lex_error(L, t->start, t->end,
                    "lexer error: %% without tag (use %%{} for the empty tag)");
            /* NOTREACHED */
        }
        t->alloclen = t->end - t->start + 1;
        t = xrealloc(t, sizeof(struct token) + t->alloclen);
        *tptr = t;
        t->value.str = (char *)(t + 1);
        memcpy(t->value.str, L->source + t->start + 1, t->alloclen - 1);
        t->value.str[t->alloclen - 1] = '\0';
    }
}


void
lex_error(struct lexer *restrict L, int start, int end, const char *fmt, ...)
{
    va_list ap;

    assert_not_null(L);

    end = end - start + 1;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    (void)fprintf(stderr, "\n%s\n", L->source);
    while (start--)
        (void)fprintf(stderr, " ");
    while (end--)
        (void)fprintf(stderr, "^");
    (void)fprintf(stderr, "\n");
    exit(EINVAL);
    /* NOTREACHED */
}


struct token *
lex(struct lexer *restrict L)
{
    struct token *t;
    /* used for keywords detection */
    char c;
    unsigned int i;
    bool found;

    assert_not_null(L);

    t = xmalloc(sizeof(struct token));

    /* check for TSTART */
    if (L->cindex == -1) {
        (void)lexc(L);
        t->kind  = TSTART;
        t->start = -1;
        t->end   = -1;

        return (t);
    }

    /* eat blank chars */
    while (L->c != '\0' && isspace(L->c))
        (void)lexc(L);

    t->start = L->cindex;
    switch(L->c) {
    case '\0':
        t->kind  = TEND;
        t->start = -1;
        t->end   = -1;
        break;
    case '!':
        if (lexc(L) == '=') {
            t->kind = TDIFF;
            t->end  = L->cindex;
            (void)lexc(L);
        }
        else {
            t->kind = TNOT;
            t->end  = L->cindex - 1;
        }
        break;
    case '=':
        switch (lexc(L)) {
        case '=':
            t->kind = TEQ;
            break;
        case '~':
            t->kind = TMATCH;
            break;
        default:
            lex_error(L, t->start, t->start,
                    "lexer error expected `==' for EQ or `=~' for MATCH");
            /* NOTREACHED */
        }
        t->end = L->cindex;
        (void)lexc(L);
        break;
    case '|':
        if (lexc(L) == '|') {
            t->kind = TOR;
            t->end  = L->cindex;
            (void)lexc(L);
        }
        else {
            lex_error(L, t->start, t->start,
                    "lexer error expected `||' for OR");
            /* NOTREACHED */
        }
        break;
    case '&':
        if (lexc(L) == '&') {
            t->kind = TAND;
            t->end  = L->cindex;
            (void)lexc(L);
        }
        else {
            lex_error(L, t->start, t->start,
                    "lexer error expected `&&' for AND");
            /* NOTREACHED */
        }
        break;
    case '<':
        if (lexc(L) == '=') {
            t->kind = TLE;
            t->end  = L->cindex;
            (void)lexc(L);
        }
        else {
            t->kind = TLT;
            t->end  = L->cindex - 1;
        }
        break;
    case '>':
        if (lexc(L) == '=') {
            t->kind = TGE;
            t->end  = L->cindex;
            (void)lexc(L);
        }
        else {
            t->kind = TGT;
            t->end  = L->cindex - 1;
        }
        break;
    case '(':
        t->kind = TOPAREN;
        t->end  = L->cindex;
        (void)lexc(L);
        break;
    case ')':
        t->kind = TCPAREN;
        t->end  = L->cindex;
        (void)lexc(L);
        break;
    case '+': /* FALLTHROUGH */
    case '-': /* FALLTHROUGH */
    case '0': /* FALLTHROUGH */
    case '1': /* FALLTHROUGH */
    case '2': /* FALLTHROUGH */
    case '3': /* FALLTHROUGH */
    case '4': /* FALLTHROUGH */
    case '5': /* FALLTHROUGH */
    case '6': /* FALLTHROUGH */
    case '7': /* FALLTHROUGH */
    case '8': /* FALLTHROUGH */
    case '9': /* FALLTHROUGH */
    case '.':
        lex_number(L, t);
        break;
    case '/': /* FALLTHROUGH */
    case '"':
        lex_strlit_or_regex(L, &t);
        break;
    case '%':
        lex_tagkey(L, &t);
        break;
    default:
        /* keywords, not optimized at all, no gperf or so */
        found = false;
        for (i = 0; !found && i < len(keywords); i++) {
            if (strncmp(L->source + L->cindex, keywords[i].lexem, keywords[i].lexemlen) == 0) {
                c = L->source[L->cindex + keywords[i].lexemlen];
                if (!isalnum(c) && c != '_') {
                    t->kind = keywords[i].kind;
                    L->cindex += keywords[i].lexemlen - 1;
                    t->end = L->cindex;
                    lexc(L);
                    found = true;
                }
            }
        }
        if (!found) {
            lex_error(L, t->start, t->start,
                    "lexer error: not a keyword");
        }
        break;
    }

    return (t);
}

#if 1
int
main(int argc, char *argv[])
{
    struct lexer *L;
    struct token *t;
    int the_end;

    if (argc < 2)
        errx(-1, "usage: %s [string]\n", argv[0]);

    the_end = 0;
    L = new_lexer(argv[1]);

    while (!the_end) {
        t = lex(L);
        (void)printf("%s", token_to_s(t->kind));

        switch (t->kind) {
            case TINT:
                (void)printf("(%d) ", t->value.integer);
                break;
            case TDOUBLE:
                (void)printf("(%lf) ", t->value.dbl);
                break;
            case TSTRING: /* FALLTHROUGH */
            case TKEYWORD:
                (void)printf("(%s)[%d] ", t->value.str, (int)t->alloclen);
                break;
            case TREGEX:
                (void)printf("(%s)[%d] ", (char *)(t + 1), (int)t->alloclen);
                break;
            case TEND:
                (void)printf("\n");
                the_end = 1;
                break;
            default:
                (void)printf(" ");
                break;
        }
        free(t);
    }

    return (0);
}
#endif

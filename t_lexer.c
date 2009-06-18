/*
 * t_lexer.c
 *
 * a hand writted lexer for tagutil.
 * used by the filter function.
 */
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_lexer.h"


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


char
lexc(struct lexer *restrict L)
{

    assert_not_null(L);

    if (L->c != '\0') {
    /* move forward of one char */
        assert(L->cindex == -1 || (size_t)L->cindex < L->srclen);
        L->cindex += 1;
        L->c = L->source[L->cindex];
    }
    else
        assert(L->cindex >= 0 && (size_t)L->cindex == L->srclen);

    return (L->c);
}


char
lexc_move(struct lexer *restrict L, int delta)
{

    assert_not_null(L);

    return (lexc_move_to(L, L->cindex + delta));
}


char
lexc_move_to(struct lexer *restrict L, int to)
{

    assert_not_null(L);

    L->cindex = to;
    assert(L->cindex == -1 || (size_t)L->cindex <= L->srclen);
    L->c = L->source[L->cindex];
    if (L->c == '\0')
        assert(L->cindex >= 0 && (size_t)L->cindex == L->srclen);

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
        t->str = "INT";
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
			t->str = "DOUBLE";
            t->value.dbl = strtod(num, &endptr);
            if (!strempty(endptr)) {
                lex_error(L, start - L->source, end - L->source,
                        "bad floating point value");
                /* NOTREACHED */
            }
        }
        else {
            t->kind = TINT;
			t->str = "INT";
            t->value.integer = (int)strtol(num, &endptr, 10);
            if (!strempty(endptr)) {
                lex_error(L, start - L->source, end - L->source,
                        "bad integer value");
                /* NOTREACHED */
            }
        }
        freex(num);
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
	if (limit == '"') {
		t->kind = TSTRING;
		t->str  = "STRING";
	}
	else {
		t->kind = TREGEX;
		t->str  = "REGEX";
	}
    while (L->c != '\0' && lexc(L) != limit) {
        if (L->c == '\\' && lexc(L) == limit)
            skip += 1;
    }

    t->end = L->cindex;
    if (L->c != limit)
        lex_error(L, t->start, t->end, "unbalanced %c for %s", limit, t->str);

    /* do the copy */
    t->slen  = t->end - t->start - skip - 1;
    t = xrealloc(t, sizeof(struct token) + t->slen + 1);
    *tptr = t;
    t->value.str = (char *)(t + 1);
    lexc_move_to(L, t->start);
    i = 0;
    while (lexc(L) != limit) {
        if (L->c == '\\') {
            if (lexc(L) != limit) {
            /* rewind */
                lexc_move(L, -1);
                assert(L->c == '\\');
            }
        }
        t->value.str[i++] = L->c;
    }
    t->value.str[i] = '\0';
    assert(strlen(t->value.str) == t->slen);

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
                    (void)fprintf(stderr, "***bug in regex option lexing***\n");
                    assert_fail();
                    /* NOTREACHED */
regopt_error:
                    lex_error(L, t->start, L->cindex,
                            "option %c given twice for REGEX", L->c);
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
                    "can't compile REGEX /%s/: %s", s, errbuf);
            /* NOTREACHED */
        }
    }
}


void
lex_tagkey(struct lexer *restrict L, struct token **tptr)
{
    bool idx;
    int copyend, eqindex;
    struct token *t;
    unsigned int skip, i;

    assert_not_null(L);
    assert_not_null(tptr);
    assert_not_null(*tptr);
    assert(L->c == '%');

    t = *tptr;
    t->kind = TTAGKEY;
    t->str = "TAGKEY";
    t->tindex = 0;

    if (lexc(L) == '{') {
    /* %{tag} */
        skip = 0;
        while (lexc(L) != '\0' && L->c != '}' && L->c != '=') {
            if (L->c == '\\') {
                if (lexc(L) == '}')
                    skip += 1;
            }
        }
        if (L->c != '=')
            idx = false;
        else {
        /* %{tag=idx} */
            idx = true;
            eqindex = L->cindex;
            while (isdigit(lexc(L)))
                t->tindex = 10 * t->tindex + digittoint(L->c);
            if (L->cindex == eqindex + 1) {
            /* could be * */
                if (L->c == '*') {
                    t->tindex = -1;
                    (void)lexc(L);
                }
                if (L->c && L->c != '}')
                    lex_error(L, eqindex + 1, L->cindex, "bad index request");
            }
        }
        t->end = L->cindex;
        if (L->c != '}')
            lex_error(L, t->start, t->end, "unbalanced { for %s", t->str);

        /* do the copy */
        copyend = idx ? eqindex : t->end;
        t->slen = copyend - t->start - 2 - skip;
        t = xrealloc(t, sizeof(struct token) + t->slen + 1);
        *tptr = t;
        t->value.str = (char *)(t + 1);
        lexc_move_to(L, t->start + 1); /* move on { */
        i = 0;
        while (lexc(L) != (idx ? '=' : '}')) {
            if (L->c == '\\') {
                if (lexc(L) != '}') {
                    /* rewind */
                    lexc_move(L, -1);
                    assert(L->c == '\\');
                }
            }
            t->value.str[i++] = L->c;
        }
        if (idx) {
        /* eat idx digits */
            while (lexc(L) != '}')
                assert(L->c);
        }
        t->value.str[i] = '\0';
        assert(strlen(t->value.str) == t->slen);
        (void)lexc(L);
    }
    else {
    /* %tag */
        while (isalnum(L->c) || L->c == '-' || L->c == '_')
            (void)lexc(L);
        t->end = L->cindex - 1;
        if (t->end == t->start) {
            lex_error(L, t->start, t->end,
                    "%% without tag (use %%{} for the empty tag)");
            /* NOTREACHED */
        }
        t->slen = t->end - t->start;
        t = xrealloc(t, sizeof(struct token) + t->slen + 1);
        *tptr = t;
        t->value.str = (char *)(t + 1);
        memcpy(t->value.str, L->source + t->start + 1, t->slen);
        t->value.str[t->slen] = '\0';
    }
}


struct token *
lex_next_token(struct lexer *restrict L)
{
    struct token *t;
    /* used for keywords detection */
    char c;
    unsigned int i;
    bool found;

    assert_not_null(L);

    t = xcalloc(1, sizeof(struct token));

    /* check for TSTART */
    if (L->cindex == -1) {
        (void)lexc(L);
        t->kind  = TSTART;
		t->str   = "START";
        L->current = t;
        return (L->current);
    }

    /* eat blank chars */
    while (L->c != '\0' && isspace(L->c))
        (void)lexc(L);

    t->start = L->cindex;
    switch (L->c) {
    case '\0':
        t->kind = TEND;
        t->str  = "END";
        t->end  = L->cindex;
        break;
    case '!':
        switch (lexc(L)) {
        case '=':
            t->kind = TDIFF;
            t->str  = "DIFF";
            t->end  = L->cindex;
            (void)lexc(L);
            break;
        case '~':
            t->kind = TNMATCH;
            t->str  = "NMATCH";
            t->end  = L->cindex;
            (void)lexc(L);
            break;
        default:
            t->kind = TNOT;
            t->str  = "NOT";
            t->end  = L->cindex - 1;
        }
        break;
    case '=':
        switch (lexc(L)) {
        case '=':
            t->kind = TEQ;
            t->str  = "EQ";
            break;
        case '~':
            t->kind = TMATCH;
            t->str  = "MATCH";
            break;
        default:
            lex_error(L, t->start, t->start,
                    "expected `==' for EQ or `=~' for MATCH");
            /* NOTREACHED */
        }
        t->end = L->cindex;
        (void)lexc(L);
        break;
    case '|':
        if (lexc(L) == '|') {
            t->kind = TOR;
            t->str  = "OR";
            t->end  = L->cindex;
            (void)lexc(L);
        }
        else {
            lex_error(L, t->start, t->start,
                    "expected `||' for OR");
            /* NOTREACHED */
        }
        break;
    case '&':
        if (lexc(L) == '&') {
            t->kind = TAND;
            t->str  = "AND";
            t->end  = L->cindex;
            (void)lexc(L);
        }
        else {
            lex_error(L, t->start, t->start,
                    "expected `&&' for AND");
            /* NOTREACHED */
        }
        break;
    case '<':
        if (lexc(L) == '=') {
            t->kind = TLE;
            t->str  = "LE";
            t->end  = L->cindex;
            (void)lexc(L);
        }
        else {
            t->kind = TLT;
            t->str  = "LT";
            t->end  = L->cindex - 1;
        }
        break;
    case '>':
        if (lexc(L) == '=') {
            t->kind = TGE;
            t->str  = "GE";
            t->end  = L->cindex;
            (void)lexc(L);
        }
        else {
            t->kind = TGT;
            t->str  = "GT";
            t->end  = L->cindex - 1;
        }
        break;
    case '(':
        t->kind = TOPAREN;
        t->str  = "OPAREN";
        t->end  = L->cindex;
        (void)lexc(L);
        break;
    case ')':
        t->kind = TCPAREN;
        t->str  = "CPAREN";
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
        for (i = 0; !found && i < countof(lexkeywords); i++) {
            if (strncmp(L->source + L->cindex,
                        lexkeywords[i].lexem,
                        lexkeywords[i].lexemlen) == 0) {
                c = L->source[L->cindex + lexkeywords[i].lexemlen];
                if (!isalnum(c) && c != '_') {
                    t->kind = lexkeywords[i].kind;
                    t->str  = lexkeywords[i].lexem;
                    lexc_move(L, lexkeywords[i].lexemlen - 1);
                    t->end = L->cindex;
                    assert(lexc(L) == c);
                    found = true;
                }
            }
        }
        if (!found) {
            lex_error(L, t->start, t->start,
                    "not a keyword");
        }
        break;
    }

    L->current = t;
    return (L->current);
}


void
lex_error0(const struct lexer *restrict L, int start, int end,
        const char *restrict fmt, va_list args)
{
    char c;

    assert_not_null(L);

    end = end - start + 1;
    c = (end == 1 ? '^' : '~');
    (void)vfprintf(stderr, fmt, args);
    (void)fprintf(stderr, "\n%s\n", L->source);
    while (start--)
        (void)fprintf(stderr, " ");
    while (end--)
        (void)fprintf(stderr, "%c", c);
    (void)fprintf(stderr, "\n");
}

void
lex_error(const struct lexer *restrict L, int start, int end,
        const char *restrict fmt, ...)
{
    va_list args;

    assert_not_null(L);

    fprintf(stderr, "lexer error: ");
    va_start(args, fmt);
        lex_error0(L, start, end, fmt, args);
    va_end(args);

    exit(EINVAL);
    /* NOTREACHED */
}


#if 0
int
token_debug(struct token *restrict t) {
    int ret = 0;
    if (t == NULL)
        return 0;

    (void)printf("%s", t->str);

    switch (t->kind) {
        case TINT:
            (void)printf("(%d)", t->value.integer);
            break;
        case TDOUBLE:
            (void)printf("(%lf)", t->value.dbl);
            break;
        case TSTRING: /* FALLTHROUGH */
            (void)printf("(%s)", t->value.str);
            break;
        case TTAGKEY:
            (void)printf("(%s@%d)", t->value.str, t->tindex);
            break;
        case TREGEX:
            (void)printf("(%s)", (char *)(t + 1));
            break;
        case TEND:
            (void)printf("\n");
            ret = 1;
            break;
        default:
            break;
    }

    return (ret);
}

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
        t = lex_next_token(L);
        the_end = token_debug(t);
        (void)printf(" ");
        freex(t);
    }
    freex(L);

    return (0);
}
#endif

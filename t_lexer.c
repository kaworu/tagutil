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


struct t_lexer *
t_lexer_new(const char *restrict source)
{
    char *s;
    size_t len;
    struct t_lexer *L;

    assert_not_null(source);

    len = strlen(source) + 1;
    L = xmalloc(sizeof(struct t_lexer) + len);

    L->srclen = len - 1;
    s = (char *)(L + 1);
    strlcpy(s, source, len);
    L->source = s;
    L->c = -1;
    L->cindex = -1;

    return (L);
}


char
t_lexc(struct t_lexer *restrict L)
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
t_lexc_move(struct t_lexer *restrict L, int delta)
{

    assert_not_null(L);

    return (t_lexc_move_to(L, L->cindex + delta));
}


char
t_lexc_move_to(struct t_lexer *restrict L, int to)
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
t_lex_number(struct t_lexer *restrict L, struct t_token *restrict t)
{
    char *num, *endptr; /* new buffer pointers */
    const char *start, *end; /* source pointers */
    bool isfp;

    assert_not_null(L);
    assert_not_null(t);
    assert(isnumber(L->c) || L->c == '.' || L->c == '+' || L->c == '-');

    start = L->source + L->cindex;
    if (L->c == '+' || L->c == '-')
        (void)t_lexc(L);

    if (L->c == '0' && t_lexc(L) != '.') {
    /* int 0 */
        t->kind = T_INT;
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
            (void)t_lexc(L);
        }
        end = L->source + L->cindex - 1;
        assert(end >= start);
        num = xcalloc((end - start + 1) + 1, sizeof(char));
        memcpy(num, start, end - start + 1);
        if (isfp) {
            t->kind = T_DOUBLE;
			t->str = "DOUBLE";
            t->value.dbl = strtod(num, &endptr);
            if (!strempty(endptr)) {
                t_lex_error(L, start - L->source, end - L->source,
                        "bad floating point value");
                /* NOTREACHED */
            }
        }
        else {
            t->kind = T_INT;
			t->str = "INT";
            t->value.integer = (int)strtol(num, &endptr, 10);
            if (!strempty(endptr)) {
                t_lex_error(L, start - L->source, end - L->source,
                        "bad integer value");
                /* NOTREACHED */
            }
        }
        freex(num);
    }
    t->end = L->cindex - 1;
}


void
t_lex_strlit_or_regex(struct t_lexer *restrict L, struct t_token  **tptr)
{
    struct t_token *t;
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
		t->kind = T_STRING;
		t->str  = "STRING";
	}
	else {
		t->kind = T_REGEX;
		t->str  = "REGEX";
	}
    while (L->c != '\0' && t_lexc(L) != limit) {
        if (L->c == '\\' && t_lexc(L) == limit)
            skip += 1;
    }

    t->end = L->cindex;
    if (L->c != limit)
        t_lex_error(L, t->start, t->end, "unbalanced %c for %s", limit, t->str);

    /* do the copy */
    t->slen  = t->end - t->start - skip - 1;
    t = xrealloc(t, sizeof(struct t_token) + t->slen + 1);
    *tptr = t;
    t->value.str = (char *)(t + 1);
    t_lexc_move_to(L, t->start);
    i = 0;
    while (t_lexc(L) != limit) {
        if (L->c == '\\') {
            if (t_lexc(L) != limit) {
            /* rewind */
                t_lexc_move(L, -1);
                assert(L->c == '\\');
            }
        }
        t->value.str[i++] = L->c;
    }
    t->value.str[i] = '\0';
    assert(strlen(t->value.str) == t->slen);

    /* skip limit */
    (void)t_lexc(L);

    /* handle regex options */
    if (t->kind == T_REGEX) {
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
                    t_lex_error(L, t->start, L->cindex,
                            "option %c given twice for REGEX", L->c);
                    /* NOTREACHED */
            }
            (void)t_lexc(L);
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
            t_lex_error(L, t->start, t->end,
                    "can't compile REGEX /%s/: %s", s, errbuf);
            /* NOTREACHED */
        }
    }
}


void
t_lex_tagkey(struct t_lexer *restrict L, struct t_token **tptr)
{
    bool idx;
    int copyend, eqindex;
    struct t_token *t;
    unsigned int skip, i;

    assert_not_null(L);
    assert_not_null(tptr);
    assert_not_null(*tptr);
    assert(L->c == '%');

    t = *tptr;
    t->kind = T_TAGKEY;
    t->str = "TAGKEY";
    t->tindex = 0;

    if (t_lexc(L) == '{') {
    /* %{tag} */
        skip = 0;
        while (t_lexc(L) != '\0' && L->c != '}' && L->c != '=') {
            if (L->c == '\\') {
                if (t_lexc(L) == '}')
                    skip += 1;
            }
        }
        if (L->c != '=')
            idx = false;
        else {
        /* %{tag=idx} */
            idx = true;
            eqindex = L->cindex;
            while (isdigit(t_lexc(L)))
                t->tindex = 10 * t->tindex + digittoint(L->c);
            if (L->cindex == eqindex + 1) {
            /* could be * */
                if (L->c == '*') {
                    t->tindex = -1;
                    (void)t_lexc(L);
                }
                if (L->c && L->c != '}')
                    t_lex_error(L, eqindex + 1, L->cindex, "bad index request");
            }
        }
        t->end = L->cindex;
        if (L->c != '}')
            t_lex_error(L, t->start, t->end, "unbalanced { for %s", t->str);

        /* do the copy */
        copyend = idx ? eqindex : t->end;
        t->slen = copyend - t->start - 2 - skip;
        t = xrealloc(t, sizeof(struct t_token) + t->slen + 1);
        *tptr = t;
        t->value.str = (char *)(t + 1);
        t_lexc_move_to(L, t->start + 1); /* move on { */
        i = 0;
        while (t_lexc(L) != (idx ? '=' : '}')) {
            if (L->c == '\\') {
                if (t_lexc(L) != '}') {
                    /* rewind */
                    t_lexc_move(L, -1);
                    assert(L->c == '\\');
                }
            }
            t->value.str[i++] = L->c;
        }
        if (idx) {
        /* eat idx digits */
            while (t_lexc(L) != '}')
                assert(L->c);
        }
        t->value.str[i] = '\0';
        assert(strlen(t->value.str) == t->slen);
        (void)t_lexc(L);
    }
    else {
    /* %tag */
        while (isalnum(L->c) || L->c == '-' || L->c == '_')
            (void)t_lexc(L);
        t->end = L->cindex - 1;
        if (t->end == t->start) {
            t_lex_error(L, t->start, t->end,
                    "%% without tag (use %%{} for the empty tag)");
            /* NOTREACHED */
        }
        t->slen = t->end - t->start;
        t = xrealloc(t, sizeof(struct t_token) + t->slen + 1);
        *tptr = t;
        t->value.str = (char *)(t + 1);
        memcpy(t->value.str, L->source + t->start + 1, t->slen);
        t->value.str[t->slen] = '\0';
    }
}


struct t_token *
t_lex_next_token(struct t_lexer *restrict L)
{
    struct t_token *t;
    /* used for keywords detection */
    char c;
    unsigned int i;
    bool found;

    assert_not_null(L);

    t = xcalloc(1, sizeof(struct t_token));

    /* check for T_START */
    if (L->cindex == -1) {
        (void)t_lexc(L);
        t->kind  = T_START;
		t->str   = "START";
        L->current = t;
        return (L->current);
    }

    /* eat blank chars */
    while (L->c != '\0' && isspace(L->c))
        (void)t_lexc(L);

    t->start = L->cindex;
    switch (L->c) {
    case '\0':
        t->kind = T_END;
        t->str  = "END";
        t->end  = L->cindex;
        break;
    case '!':
        switch (t_lexc(L)) {
        case '=':
            t->kind = T_DIFF;
            t->str  = "DIFF";
            t->end  = L->cindex;
            (void)t_lexc(L);
            break;
        case '~':
            t->kind = T_NMATCH;
            t->str  = "NMATCH";
            t->end  = L->cindex;
            (void)t_lexc(L);
            break;
        default:
            t->kind = T_NOT;
            t->str  = "NOT";
            t->end  = L->cindex - 1;
        }
        break;
    case '=':
        switch (t_lexc(L)) {
        case '=':
            t->kind = T_EQ;
            t->str  = "EQ";
            break;
        case '~':
            t->kind = T_MATCH;
            t->str  = "MATCH";
            break;
        default:
            t_lex_error(L, t->start, t->start,
                    "expected `==' for EQ or `=~' for MATCH");
            /* NOTREACHED */
        }
        t->end = L->cindex;
        (void)t_lexc(L);
        break;
    case '|':
        if (t_lexc(L) == '|') {
            t->kind = T_OR;
            t->str  = "OR";
            t->end  = L->cindex;
            (void)t_lexc(L);
        }
        else {
            t_lex_error(L, t->start, t->start,
                    "expected `||' for OR");
            /* NOTREACHED */
        }
        break;
    case '&':
        if (t_lexc(L) == '&') {
            t->kind = T_AND;
            t->str  = "AND";
            t->end  = L->cindex;
            (void)t_lexc(L);
        }
        else {
            t_lex_error(L, t->start, t->start,
                    "expected `&&' for AND");
            /* NOTREACHED */
        }
        break;
    case '<':
        if (t_lexc(L) == '=') {
            t->kind = T_LE;
            t->str  = "LE";
            t->end  = L->cindex;
            (void)t_lexc(L);
        }
        else {
            t->kind = T_LT;
            t->str  = "LT";
            t->end  = L->cindex - 1;
        }
        break;
    case '>':
        if (t_lexc(L) == '=') {
            t->kind = T_GE;
            t->str  = "GE";
            t->end  = L->cindex;
            (void)t_lexc(L);
        }
        else {
            t->kind = T_GT;
            t->str  = "GT";
            t->end  = L->cindex - 1;
        }
        break;
    case '(':
        t->kind = T_OPAREN;
        t->str  = "OPAREN";
        t->end  = L->cindex;
        (void)t_lexc(L);
        break;
    case ')':
        t->kind = T_CPAREN;
        t->str  = "CPAREN";
        t->end  = L->cindex;
        (void)t_lexc(L);
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
        t_lex_number(L, t);
        break;
    case '/': /* FALLTHROUGH */
    case '"':
        t_lex_strlit_or_regex(L, &t);
        break;
    case '%':
        t_lex_tagkey(L, &t);
        break;
    default:
        /* keywords, not optimized at all, no gperf or so */
        found = false;
        for (i = 0; !found && i < countof(t_lex_keywords_table); i++) {
            if (strncmp(L->source + L->cindex,
                        t_lex_keywords_table[i].lexem,
                        t_lex_keywords_table[i].lexemlen) == 0) {
                c = L->source[L->cindex + t_lex_keywords_table[i].lexemlen];
                if (!isalnum(c) && c != '_') {
                    t->kind = t_lex_keywords_table[i].kind;
                    t->str  = t_lex_keywords_table[i].lexem;
                    t_lexc_move(L, t_lex_keywords_table[i].lexemlen - 1);
                    t->end = L->cindex;
                    assert(t_lexc(L) == c);
                    found = true;
                }
            }
        }
        if (!found) {
            t_lex_error(L, t->start, t->start,
                    "not a keyword");
        }
        break;
    }

    L->current = t;
    return (L->current);
}


void
t_lex_error0(const struct t_lexer *restrict L, int start, int end,
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
t_lex_error(const struct t_lexer *restrict L, int start, int end,
        const char *restrict fmt, ...)
{
    va_list args;

    assert_not_null(L);

    fprintf(stderr, "lexer error: ");
    va_start(args, fmt);
        t_lex_error0(L, start, end, fmt, args);
    va_end(args);

    exit(EINVAL);
    /* NOTREACHED */
}


#if 0
int
t_lex_token_debug(struct t_token *restrict t) {
    int ret = 0;
    if (t == NULL)
        return 0;

    (void)printf("%s", t->str);

    switch (t->kind) {
        case T_INT:
            (void)printf("(%d)", t->value.integer);
            break;
        case T_DOUBLE:
            (void)printf("(%lf)", t->value.dbl);
            break;
        case T_STRING: /* FALLTHROUGH */
            (void)printf("(%s)", t->value.str);
            break;
        case T_TAGKEY:
            (void)printf("(%s@%d)", t->value.str, t->tindex);
            break;
        case T_REGEX:
            (void)printf("(%s)", (char *)(t + 1));
            break;
        case T_END:
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
    struct t_lexer *L;
    struct t_token *t;
    int the_end;

    if (argc < 2)
        errx(-1, "usage: %s [string]\n", argv[0]);

    the_end = 0;
    L = t_lexer_new(argv[1]);

    while (!the_end) {
        t = t_lex_next_token(L);
        the_end = t_lex_token_debug(t);
        (void)printf(" ");
        freex(t);
    }
    freex(L);

    return (0);
}
#endif

#ifndef T_LEXER_H
#define T_LEXER_H
/*
 * t_lexer.h
 *
 * a hand writted lexer for tagutil.
 * used by the filter function.
 */
#include "t_config.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <regex.h>

#include "t_toolkit.h"


enum tokenkind {
    TEND,
    TSTART,

    /* unary op */
    TNOT,

    /* binary op */
    TEQ,
    TDIFF,
    TMATCH,
    TNMATCH,
    TLT,
    TLE,
    TGT,
    TGE,
    TAND,
    TOR,

    TOPAREN,
    TCPAREN,

    /* types */
    TINT,
    TDOUBLE,
    TSTRING,
    TREGEX,
    TFILENAME,
    TBACKEND,
    TUNDEF,
    TTAGKEY
};


_t__unused
static const struct {
    enum tokenkind kind;
    const char *lexem;
    const size_t lexemlen;
} lexkeywords[] = {
    { TFILENAME, "filename", 8 },
    { TUNDEF,    "undef",    5 },
    { TBACKEND,  "backend",  7 },
};


struct token {
    const char *str;
    enum tokenkind kind;
	int start, end;
    union {
        int integer;   /* TINT */
        double dbl;    /* TDOUBLE */
        char *str;     /* TSTRING or TTAGKEY  */
        regex_t regex; /* TREGEX */
    } value;
    size_t slen; /* > 0 if TSTRING or TTAGKEY */
};

struct lexer {
	size_t srclen;
    const char *source;
    char c; /* current char */
    int cindex;  /* index of c in source */
    struct token *current;
};


/*
 * create a new lexer with given inpute source.
 *
 * returned value has to be free()d.
 */
_t__nonnull(1)
struct lexer * new_lexer(const char *restrict source);

/*
 * return the next token of given lexer. The first token is always TSTART, and
 * at the end TEND is always returned when there is no more token.
 *
 * The returned value of the function is L->current, and it has to be free()d.
 */
_t__nonnull(1)
struct token * lex_next_token(struct lexer *restrict L);

/*
 * put next char in L->c and increment L->cindex.
 */
_t__nonnull(1)
char lexc(struct lexer *restrict L);

/*
 * fill given token with TINT or TDOUBLE.
 */
_t__nonnull(1) _t__nonnull(2)
void lex_number(struct lexer *restrict L, struct token *restrict t);

/*
 * realloc() given token and fill it with TSTRING or TREGEX.
 */
_t__nonnull(1) _t__nonnull(2)
void lex_strlit_or_regex(struct lexer *restrict L, struct token **tptr);

/*
 * realloc() given token and fill it with TTAGKEY;
 */
_t__nonnull(1) _t__nonnull(2)
void lex_tagkey(struct lexer *restrict L, struct token **tptr);

/*
 * output nicely lexer's error messages and die.
 */
_t__nonnull(1) _t__dead2 _t__printflike(4, 5)
void lex_error(const struct lexer *restrict L, int start, int size,
        const char *restrict fmt, ...);

/*
 * output the fmt error message and the source string underlined from start to
 * end.
 */
_t__nonnull(1)
void lex_error0(const struct lexer *restrict L, int start, int end,
        const char *restrict fmt, va_list args);
#endif /* not T_LEXER_H */

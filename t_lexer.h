#ifndef T_LEXER_H
#define T_LEXER_H
/*
 * t_lexer.c
 *
 * a hand writted lexer for tagutil.
 * used by the filter function.
 */
#include "t_config.h"

#include <ctype.h>
#include <stdlib.h>
#include <regex.h>


enum tokenkind {
    TEND,
    TSTART,

    /* unary op */
    TNOT,

    /* binary op */
    TEQ,
    TDIFF,
    TMATCH,
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
    TUNDEF,
    TKEYWORD
};


_t__unused
static const char *_tokens[] = {
    [TNOT]      = "TNOT",
    [TEQ]       = "TEQ",
    [TDIFF]     = "TDIFF",
    [TMATCH]    = "TMATCH",
    [TLT]       = "TLT",
    [TLE]       = "TLE",
    [TGT]       = "TGT",
    [TGE]       = "TGE",
    [TAND]      = "TAND",
    [TOR]       = "TOR",
    [TOPAREN]   = "TOPAREN",
    [TCPAREN]   = "TCPAREN",
    [TINT]      = "TINT",
    [TDOUBLE]   = "TDOUBLE",
    [TSTRING]   = "TSTRING",
    [TREGEX]    = "TREGEX",
    [TFILENAME] = "TFILENAME",
    [TUNDEF]	= "TUNDEF",
    [TKEYWORD]  = "TKEYWORD",
    [TEND]      = "TEND",
    [TSTART]    = "TSTART"
};

#define token_to_s(x) _tokens[(x)]


struct token {
    enum tokenkind kind;
	int start, end;
    union {
        int integer;   /* TINT */
        double dbl;    /* TDOUBLE */
        char *str;     /* TSTRING ou TKEYWORD */
        regex_t regex; /* TREGEX */
    } value;
    size_t alloclen; /* > 0 if TSTRING or TKEYWORD */
};

struct key {
    enum tokenkind kind;
    const char *lexem;
    const size_t lexemlen;
};

/* keyword table */
static const struct key keywords[] = {
    { TFILENAME, "FILENAME", 8 },
};

struct lexer {
	size_t srclen;
    const char *source;
    char c; /* current char */
    int cindex;  /* index of c in source */
};


/*
 * create a new lexer with given inpute source.
 *
 * returned value need to be free()d.
 */
_t__nonnull(1)
struct lexer * new_lexer(const char *restrict source);

/*
 * return the next token of given lexer. The first token is always TSTART, and
 * at the end TEND is always returned when there is no more token.
 *
 * returned value need to be free()d.
 */
_t__nonnull(1)
struct token * lex(struct lexer *restrict L);

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
 * realloc() given token and fill it with TKEYWORD;
 */
_t__nonnull(1) _t__nonnull(2)
void lex_tagkey(struct lexer *restrict L, struct token **tptr);

/*
 * TODO
 */
_t__nonnull(1) _t__dead2 _t__printflike(4, 5)
void lex_error(struct lexer *restrict L, int start, int size, const char *fmt, ...);
#endif /* not T_LEXER_H */

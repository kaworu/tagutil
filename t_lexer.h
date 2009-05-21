#ifndef T_LEXER_H
#define T_LEXER_H
/*
 * t_lexer.c
 *
 * a hand writted lexer for tagutil.
 * used by the filter function.
 */
#include "t_config.h"

#include <stdlib.h>
#include <regex.h>


#define is_letter(c) ((c) >= 'a' && (c) <= 'z')
#define is_digit(i)  ((i) >= '0' && (i) <= '9')
#define is_blank(c)  ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')


enum tokenkind {
    TFILENAME,
    TTITLE,
    TALBUM,
    TARTIST,
    TYEAR,
    TTRACK,
    TCOMMENT,
    TGENRE,

    TNOT,

    TEQ,
    TDIFF,
    TMATCH,
    TAND,
    TOR,
    TLT,
    TLE,
    TGT,
    TGE,

    TOPAREN,
    TCPAREN,

    TINT,
    TSTRING,
    TREGEX,
    TEOS,
    TSTART,
};


_t__unused
static const char *_tokens[] = {
    [TFILENAME] = "TFILENAME",
    [TTITLE]    = "TTITLE",
    [TALBUM]    = "TALBUM",
    [TARTIST]   = "TARTIST",
    [TYEAR]     = "TYEAR",
    [TTRACK]    = "TTRACK",
    [TCOMMENT]  = "TCOMMENT",
    [TGENRE]    = "TGENRE",
    [TNOT]      = "TNOT",
    [TEQ]       = "TEQ",
    [TDIFF]     = "TDIFF",
    [TMATCH]    = "TMATCH",
    [TAND]      = "TAND",
    [TOR]       = "TOR",
    [TLT]       = "TLT",
    [TLE]       = "TLE",
    [TGT]       = "TGT",
    [TGE]       = "TGE",
    [TOPAREN]   = "TOPAREN",
    [TCPAREN]   = "TCPAREN",
    [TINT]      = "TINT",
    [TSTRING]   = "TSTRING",
    [TREGEX]    = "TREGEX",
    [TEOS]      = "TEOS",
    [TSTART]    = "TSTART",
};

#define token_to_s(x) _tokens[(x)]

struct token {
    enum tokenkind kind;
    int start, end;
    union {
        char *string;   /* defined if kind == TSTRING */
        int integer;    /* defined if kind == TINT    */
        regex_t regex;  /* defined if tkind == TREGEX */
    } value;
    size_t alloclen; /* defined if kind == TSTRING */
};

struct key {
    enum tokenkind kind;
    const char *lexem;
    const size_t lexemlen;
};

/* keyword table */
static const struct key keywords[] = {
    { TFILENAME,    "filename", 8 },
    { TTITLE,       "title",    5 },
    { TALBUM,       "album",    5 },
    { TARTIST,      "artist",   6 },
    { TYEAR,        "year",     4 },
    { TTRACK,       "track",    5 },
    { TCOMMENT,     "comment",  7 },
    { TGENRE,       "genre",    5 },
};

struct lexer {
    const char *source;
    int index;
    struct token current;
};


_t__nonnull(1)
struct lexer * new_lexer(const char *restrict str);

_t__nonnull(1)
void lex(struct lexer *restrict L);

#endif /* not T_LEXER_H */

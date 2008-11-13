#ifndef T_LEXER_H
#define T_LEXER_H
/*
 * t_lexer.c
 *
 * a hand writted lexer for tagutil.
 * used by the filter function.
 */

#include <stdlib.h>
#include <regex.h>

#include "t_config.h"


#define is_letter(c) ((c) >= 'a' && (c) <= 'z')
#define is_number(i) ((i) >= '0' && (i) <= '9')
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


#define token_to_s(x) ( \
    (x) == TFILENAME ? "TFILENAME" : \
    (x) == TTITLE ? "TTITLE" : \
    (x) == TALBUM ? "TALBUM" : \
    (x) == TARTIST ? "TARTIST" : \
    (x) == TYEAR ? "TYEAR" : \
    (x) == TTRACK ? "TTRACK" : \
    (x) == TCOMMENT ? "TCOMMENT" : \
    (x) == TGENRE ? "TGENRE" : \
    (x) == TNOT ? "TNOT" : \
    (x) == TEQ ? "TEQ" : \
    (x) == TDIFF ? "TDIFF" : \
    (x) == TMATCH ? "TMATCH" : \
    (x) == TAND ? "TAND" : \
    (x) == TOR ? "TOR" : \
    (x) == TLT ? "TLT" : \
    (x) == TLE ? "TLE" : \
    (x) == TGT ? "TGT" : \
    (x) == TGE ? "TGE" : \
    (x) == TOPAREN ? "TOPAREN" : \
    (x) == TCPAREN ? "TCPAREN" : \
    (x) == TINT ? "TINT" : \
    (x) == TSTRING ? "TSTRING" : \
    (x) == TREGEX ? "TREGEX" : \
    (x) == TEOS ? "TEOS" : \
    (x) == TSTART ? "TSTART" : \
    "UNKNOWN" \
)

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


__t__nonnull(1)
struct lexer * new_lexer(const char *restrict str);

__t__nonnull(1)
void lex(struct lexer *restrict L);

#endif /* not T_LEXER_H */

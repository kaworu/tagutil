#ifndef T_LEXER_H
#define T_LEXER_H
/*
 * t_lexer.h
 *
 * a hand writted lexer for tagutil.
 * used by the filter function.
 */
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <regex.h>

#include "t_config.h"


enum t_tokenkind {
    T_END,
    T_START,

    /* unary op */
    T_NOT,

    /* binary op */
    T_EQ,
    T_NE,
    T_MATCH,
    T_NMATCH,
    T_LT,
    T_LE,
    T_GT,
    T_GE,
    T_AND,
    T_OR,

    T_OPAREN,
    T_CPAREN,

    /* types */
    T_INT,
    T_DOUBLE,
    T_STRING,
    T_REGEX,
    T_FILENAME,
    T_BACKEND,
    T_UNDEF,
    T_TAGKEY,
};


_t__unused
static const struct {
    enum t_tokenkind kind;
    const char *lexem;
    const size_t lexemlen;
} t_lex_keywords_table[] = {
    { T_FILENAME, "filename", 8 },
    { T_UNDEF,    "undef",    5 },
    { T_BACKEND,  "backend",  7 },
};


#define T_TOKEN_STAR (-1)
enum t_star_mod {
    T_TOKEN_STAR_NO_MOD,
    T_TOKEN_STAR_AND_MOD,
    T_TOKEN_STAR_OR_MOD,
};

struct t_token {
    const char *str;
    enum t_tokenkind kind;
	int start, end;
    union {
        int integer;   /* T_INT */
        double dbl;    /* T_DOUBLE */
        char *str;     /* T_STRING or T_TAGKEY  */
        regex_t regex; /* T_REGEX */
    } value;
    size_t slen; /* > 0 if T_STRING or T_TAGKEY */
    int tidx; /* tag index if T_TAGKEY and has_tidx (-1 is star (wildchar)) */
    enum t_star_mod tidx_mod; /* star modifier if T_TAGKEY and tidx == T_TOKEN_STAR,
                                 default is T_TOKEN_STAR_NO_MOD */
};

struct t_lexer {
	size_t srclen;
    const char *source;
    char c; /* current char */
    int cindex;  /* index of c in source */
    struct t_token *current;
};


/*
 * create a new lexer with given inpute source.
 *
 * returned value has to be free()d (see t_lexer_destroy).
 */
_t__nonnull(1)
struct t_lexer * t_lexer_new(const char *source);

/*
 * free a lexer.
 */
_t__nonnull(1)
void t_lexer_destroy(struct t_lexer *L);

/*
 * return the next token of given lexer. The first token is always T_START, and
 * at the end T_END is always returned when there is no more token.
 *
 * The returned value of the function is L->current, and it has to be free()d.
 */
_t__nonnull(1)
struct t_token * t_lex_next_token(struct t_lexer *L);

/*
 * put next char in L->c and increment L->cindex.
 */
_t__nonnull(1)
char t_lexc(struct t_lexer *L);

/*
 * move to newcindex.
 *
 * newcindex must be > 0 and <= L->srclen
 */
_t__nonnull(1)
char t_lexc_move_to(struct t_lexer *L, int to);

/*
 * move to L->index + delta.
 */
_t__nonnull(1)
char t_lexc_move(struct t_lexer *L, int delta);


/*
 * fill given token with T_INT or T_DOUBLE.
 */
_t__nonnull(1) _t__nonnull(2)
void t_lex_number(struct t_lexer *L, struct t_token *t);

/*
 * realloc() given token and fill it with T_STRING or T_REGEX.
 */
_t__nonnull(1) _t__nonnull(2)
void t_lex_strlit_or_regex(struct t_lexer *L, struct t_token **tptr);

/*
 * realloc() given token and fill it with T_TAGKEY.
 * TODO allow_star_modifier
 */
_t__nonnull(1) _t__nonnull(2)
void t_lex_tagkey(struct t_lexer *L, struct t_token **tptr,
        bool allow_star_modifier);
#define T_LEXER_ALLOW_STAR_MOD true

/*
 * helper for t_lex_tagkey.
 * lex [idx] and leave L->cindex right after the closing ].
 */
_t__nonnull(1) _t__nonnull(2)
void t_lex_tagidx(struct t_lexer *L, struct t_token *t,
        bool allow_star_modifier);

/*
 * output nicely lexer's error messages and die.
 */
_t__nonnull(1) _t__dead2 _t__printflike(4, 5)
void t_lex_error(const struct t_lexer *L, int start, int size,
        const char *fmt, ...);

/*
 * output the fmt error message and the source string underlined from start to
 * end.
 */
_t__nonnull(1)
void t_lex_error0(const struct t_lexer *L, int start, int end,
        const char *fmt, va_list args);
#endif /* not T_LEXER_H */

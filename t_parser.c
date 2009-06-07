/*
 * t_parser.c
 *
 * a LL(1) recursive descend parser for tagutil.
 * used by the filter function.
 */
#include "t_config.h"

#include <stdbool.h>
#include <string.h>

#include "t_lexer.h"
#include "t_parser.h"
#include "t_toolkit.h"


_t__nonnull(2)
static inline struct ast * new_ast(struct ast *restrict lhs,
        struct token *restrict t, struct ast *restrict rhs);

_t__nonnull(1) _t__nonnull(2) _t__nonnull(3)
_t__dead2 _t__printflike(4, 5)
void parse_error(const struct lexer *restrict L,
        const struct token *restrict start,
        const struct token *restrict end,
        const char *fmt, ...);

/* private parse_filter helpers. */
_t__nonnull(1)
static struct ast * parse_condition(struct lexer *restrict L);

/*
 * Condition ::= <Condition> '||' <Condition>
 */
_t__nonnull(1)
static struct ast * parse_or(struct lexer *restrict L);

/*
 * Condition ::= <Condition> '&&' <Condition>
 */
_t__nonnull(1)
static struct ast * parse_and(struct lexer *restrict L);

/*
 * choose between:
 *
 * 1) Condition ::= '!' '(' <Condition> ')'
 * 2) Condition ::= '(' <Condition> ')'
 * 3) Condition ::= <Value> ( '==' | '<' | '<=' | '>' | '>=' | '!=' ) <Value>
 *    Condition ::= <Value> ( '=~' | '!~' ) <REGEX>
 *    Condition ::= <REGEX> ( '=~' | '!~' ) <Value>
 */
_t__nonnull(1)
static struct ast * parse_simple(struct lexer *restrict L);

/*
 * 1) Condition ::= '!' '(' <Condition> ')'
 */
_t__nonnull(1)
static struct ast * parse_not(struct lexer *restrict L);

/*
 * 2) Condition ::= '(' <Condition> ')'
 */
_t__nonnull(1)
static struct ast * parse_nestedcond(struct lexer *restrict L);

/*
 * 3) Condition ::= <Value> ( '==' | '<' | '<=' | '>' | '>=' | '!=' ) <Value>
 *    Condition ::= <Value> ( '=~' | '!~' ) <REGEX>
 *    Condition ::= <REGEX> ( '=~' | '!~' ) <Value>
 */
_t__nonnull(1)
static struct ast *
parse_cmp_or_match_or_value(struct lexer *restrict L);


static inline struct ast *
new_ast(struct ast *restrict lhs, struct token *restrict t,
        struct ast *restrict rhs)
{
    struct ast *ret;

    assert_not_null(t);

    ret = xmalloc(sizeof(struct ast));
    ret->lhs = lhs;
    ret->rhs = rhs;
    ret->token = t;

    ret->start = (lhs ? lhs->start : t->start);
    ret->end   = (rhs ? rhs->end   : t->end  );

    return (ret);
}


void
destroy_ast(struct ast *restrict victim)
{

    if (victim != NULL) {
        destroy_ast(victim->lhs);
        destroy_ast(victim->rhs);
        if (victim->token->kind == TREGEX)
            regfree(&victim->token->value.regex);
        xfree(victim->token);
        xfree(victim);
    }
}


struct ast *
parse_filter(struct lexer *restrict L)
{
    struct ast *ret;
    struct token *t;

    assert_not_null(L);

    t = lex_next_token(L);
    if (t->kind != TSTART) {
        parse_error(L, t, t, "expected START to begin, got %s",
                t->str);
        /* NOTREACHED */
    }
    xfree(t);

    (void)lex_next_token(L);
    ret = parse_condition(L);

    t = L->current;
    if (t->kind != TEND) {
        parse_error(L, t, t, "expected END at the end, got %s",
                t->str);
        /* NOTREACHED */
    }
    xfree(t);

    xfree(L);
    return (ret);
}


static struct ast *
parse_condition(struct lexer *restrict L)
{

    assert_not_null(L);

    return (parse_or(L));
}


static struct ast *
parse_or(struct lexer *restrict L)
{
    struct token *or;
    struct ast *ret;

    assert_not_null(L);

    ret = parse_and(L);
    while (L->current->kind == TOR) {
        or = L->current;
        (void)lex_next_token(L);
        ret = new_ast(ret, or, parse_and(L));
    }

    return (ret);
}


struct ast *
parse_and(struct lexer *restrict L)
{
    struct token *and;
    struct ast *ret;

    assert_not_null(L);

    ret = parse_simple(L);
    while (L->current->kind == TAND) {
        and = L->current;
        (void)lex_next_token(L);
        ret = new_ast(ret, and, parse_simple(L));
    }

    return (ret);
}


static struct ast *
parse_simple(struct lexer *restrict L)
{
    struct token *t;
    struct ast *ret;

    assert_not_null(L);

    t = L->current;
    switch (t->kind) {
    case TNOT:
        ret = parse_not(L);
        break;
    case TOPAREN:
        ret = parse_nestedcond(L);
        break;
    case TINT:      /* FALLTHROUGH */
    case TDOUBLE:   /* FALLTHROUGH */
    case TSTRING:   /* FALLTHROUGH */
    case TREGEX:    /* FALLTHROUGH */
    case TFILENAME: /* FALLTHROUGH */
    case TUNDEF:    /* FALLTHROUGH */
    case TTAGKEY:
        ret = parse_cmp_or_match_or_value(L);
        break;
    default:
        parse_error(L, t, t,"expected  NOT, OPAREN, REGEX or value, got %s",
                t->str);
        /* NOTREACHED */
    }

    return (ret);
}


static struct ast *
parse_not(struct lexer *restrict L)
{
    struct token *not;
    struct ast *cond, *ret;

    assert_not_null(L);

    not = L->current;
    if (not->kind != TNOT)
        parse_error(L, not, not, "expected NOT, got %s", not->str);

    (void)lex_next_token(L);
    cond = parse_nestedcond(L);
    ret = new_ast(NULL, not, cond);

    return (ret);
}


static struct ast *
parse_nestedcond(struct lexer *restrict L)
{
    struct token *oparen, *cparen;
    struct ast *ret;

    assert_not_null(L);

    oparen = L->current;
    if (oparen->kind != TOPAREN) {
        parse_error(L, oparen, oparen, "expected OPAREN, got %s",
                oparen->str);
        /* NOTREACHED */
    }

    (void)lex_next_token(L);
    ret = parse_condition(L);

    cparen = L->current;
    if (cparen->kind != TCPAREN) {
        parse_error(L, oparen, cparen, "expected CPAREN, got %s",
                cparen->str);
        /* NOTREACHED */
    }
    (void)lex_next_token(L);

    ret->start = oparen->start;
    ret->end   = cparen->end;
    xfree(oparen);
    xfree(cparen);
    return (ret);
}


/*
 * 3) Condition ::= <Value> ( '==' | '<' | '<=' | '>' | '>=' | '!=' ) <Value>
 *    Condition ::= <Value> ( '=~' | '!~' ) <REGEX>
 *    Condition ::= <REGEX> ( '=~' | '!~' ) <Value>
 */
static struct ast *
parse_cmp_or_match_or_value(struct lexer *restrict L)
{
    struct token *lhstok, *optok, *rhstok;

    assert_not_null(L);
    lhstok = L->current;
    switch (lhstok->kind) {
    case TINT:      /* FALLTHROUGH */
    case TDOUBLE:   /* FALLTHROUGH */
    case TSTRING:   /* FALLTHROUGH */
    case TREGEX:    /* FALLTHROUGH */
    case TFILENAME: /* FALLTHROUGH */
    case TUNDEF:    /* FALLTHROUGH */
    case TTAGKEY:
        break;
    default:
        parse_error(L, lhstok, lhstok, "expected REGEX or value, got %s",
                lhstok->str);
        /* NOTREACHED */
    }

    optok = lex_next_token(L);
    switch (optok->kind) {
    case TEQ:     /* FALLTHROUGH */
    case TDIFF:   /* FALLTHROUGH */
    case TMATCH:  /* FALLTHROUGH */
    case TNMATCH: /* FALLTHROUGH */
    case TLT:     /* FALLTHROUGH */
    case TLE:     /* FALLTHROUGH */
    case TGT:     /* FALLTHROUGH */
    case TGE:     /* FALLTHROUGH */
        break;
    default:
        parse_error(L, lhstok, optok, "expected <value operator value>, got %s %s",
                lhstok->str, optok->str);
        /* NOTREACHED */
    }

    rhstok = lex_next_token(L);
    switch (rhstok->kind) {
    case TINT:      /* FALLTHROUGH */
    case TDOUBLE:   /* FALLTHROUGH */
    case TSTRING:   /* FALLTHROUGH */
    case TREGEX:    /* FALLTHROUGH */
    case TFILENAME: /* FALLTHROUGH */
    case TUNDEF:    /* FALLTHROUGH */
    case TTAGKEY:
        break;
    default:
        parse_error(L, rhstok, rhstok, "expected REGEX or value, got %s",
                rhstok->str);
        /* NOTREACHED */
    }

    if (optok->kind == TMATCH || optok->kind == TNMATCH) {
    /*
     * Condition ::= <Value> ( '=~' | '!~' ) <REGEX>
     * Condition ::= <REGEX> ( '=~' | '!~' ) <Value>
     */
        if ((lhstok->kind == TREGEX && rhstok->kind == TREGEX) ||
                (lhstok->kind != TREGEX && rhstok->kind != TREGEX)) {
            parse_error(L, lhstok, rhstok, "expected <REGEX (N)MATCH value> or"
                    " <value (N)MATCH REGEX>, got %s %s %s",
                    lhstok->str, optok->str, rhstok->str);
            /* NOTREACHED */
        }
    }
    else {
    /* Condition ::= <Value> ( '==' | '<' | '<=' | '>' | '>=' | '!=' ) <Value> */
        if (lhstok->kind == TREGEX || rhstok->kind == TREGEX) {
            parse_error(L, lhstok, rhstok, "unexpected REGEX (not a MATCH operator!),"
                    " got %s %s %s", lhstok->str, optok->str, rhstok->str);
            /* NOTREACHED */
        }
    }

    /* avoid constant evaluation */
    if (lhstok->kind != TTAGKEY && lhstok->kind != TFILENAME &&
            rhstok->kind != TTAGKEY && rhstok->kind != TFILENAME) {
        parse_error(L, lhstok, rhstok, "constant comparison: %s %s %s",
                lhstok->str, optok->str, rhstok->str);
        /* NOTREACHED */
    }

    (void)lex_next_token(L);
    return (new_ast(new_ast(NULL, lhstok, NULL), optok, new_ast(NULL, rhstok, NULL)));
}


void
parse_error(const struct lexer *restrict L,
        const struct token *restrict startt, const struct token *restrict endt,
        const char *fmt, ...)
{
    va_list args;

    assert_not_null(L);
    assert_not_null(startt);
    assert_not_null(endt);

    fprintf(stderr, "parser error: ");
    va_start(args, fmt);
        lex_error0(L, startt->start, endt->end, fmt, args);
    va_end(args);

    exit(EINVAL);
    /* NOTREACHED */
}


#if 0
void
ast_debug(struct ast *restrict AST)
{

    if (AST == NULL)
        return;

    token_debug(AST->token);
    if (AST->rhs) {
        (void)printf("[");
        if (AST->lhs) {
            ast_debug(AST->lhs);
            (void)printf(", ");
        }
        ast_debug(AST->rhs);
        (void)printf("]");
    }
}

int
main(int argc, char *argv[]) {
    struct ast *AST;

    if (argc < 2)
        errx(-1, "usage: %s [conditions]\n", argv[0]);

    AST = parse_filter(new_lexer(argv[1]));

    ast_debug(AST);
    (void)printf("\n");
    destroy_ast(AST);

    return (0);
}
#endif

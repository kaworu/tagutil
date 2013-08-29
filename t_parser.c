/*
 * t_parser.c
 *
 * a LL(1) recursive descend parser for tagutil.
 * used by the filter function.
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_lexer.h"
#include "t_parser.h"


_t__nonnull(2)
static struct t_ast * t_ast_new(struct t_ast *lhs,
        struct t_token *t, struct t_ast *rhs);

_t__nonnull(1) _t__nonnull(2) _t__nonnull(3)
_t__dead2 _t__printflike(4, 5)
void parse_error(const struct t_lexer *L,
        const struct t_token *start,
        const struct t_token *end,
        const char *fmt, ...);

/* private t_parse_filter helpers. */
_t__nonnull(1)
static struct t_ast * t_parse_condition(struct t_lexer *L);

/*
 * Condition ::= <Condition> '||' <Condition>
 */
_t__nonnull(1)
static struct t_ast * t_parse_or(struct t_lexer *L);

/*
 * Condition ::= <Condition> '&&' <Condition>
 */
_t__nonnull(1)
static struct t_ast * t_parse_and(struct t_lexer *L);

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
static struct t_ast * t_parse_simple(struct t_lexer *L);

/*
 * 1) Condition ::= '!' '(' <Condition> ')'
 */
_t__nonnull(1)
static struct t_ast * t_parse_not(struct t_lexer *L);

/*
 * 2) Condition ::= '(' <Condition> ')'
 */
_t__nonnull(1)
static struct t_ast * t_parse_nestedcond(struct t_lexer *L);

/*
 * 3) Condition ::= <Value> ( '==' | '<' | '<=' | '>' | '>=' | '!=' ) <Value>
 *    Condition ::= <Value> ( '=~' | '!~' ) <REGEX>
 *    Condition ::= <REGEX> ( '=~' | '!~' ) <Value>
 */
_t__nonnull(1)
static struct t_ast *
parse_cmp_or_match_or_value(struct t_lexer *L);


static struct t_ast *
t_ast_new(struct t_ast *lhs, struct t_token *t,
        struct t_ast *rhs)
{
    struct t_ast *ret;

    assert_not_null(t);

    ret = xmalloc(sizeof(struct t_ast));
    ret->lhs = lhs;
    ret->rhs = rhs;
    ret->token = t;

    ret->start = (lhs ? lhs->start : t->start);
    ret->end   = (rhs ? rhs->end   : t->end  );

    return (ret);
}


void
t_ast_destroy(struct t_ast *victim)
{

    if (victim != NULL) {
        t_ast_destroy(victim->lhs);
        t_ast_destroy(victim->rhs);
        if (victim->token->kind == T_REGEX)
            regfree(&victim->token->val.regex);
        freex(victim->token);
        freex(victim);
    }
}


struct t_ast *
t_parse_filter(struct t_lexer *L)
{
    struct t_ast *ret;
    struct t_token *t;

    assert_not_null(L);

    t = t_lex_next_token(L);
    if (t->kind != T_START) {
        parse_error(L, t, t, "expected START to begin, got %s",
                t->str);
        /* NOTREACHED */
    }
    freex(t);

    (void)t_lex_next_token(L);
    ret = t_parse_condition(L);

    t = L->current;
    if (t->kind != T_END) {
        parse_error(L, t, t, "expected END at the end, got %s",
                t->str);
        /* NOTREACHED */
    }
    freex(t);

    t_lexer_destroy(L);
    return (ret);
}


static struct t_ast *
t_parse_condition(struct t_lexer *L)
{

    assert_not_null(L);

    return (t_parse_or(L));
}


static struct t_ast *
t_parse_or(struct t_lexer *L)
{
    struct t_token *or;
    struct t_ast *ret;

    assert_not_null(L);

    ret = t_parse_and(L);
    while (L->current->kind == T_OR) {
        or = L->current;
        (void)t_lex_next_token(L);
        ret = t_ast_new(ret, or, t_parse_and(L));
    }

    return (ret);
}


struct t_ast *
t_parse_and(struct t_lexer *L)
{
    struct t_token *and;
    struct t_ast *ret;

    assert_not_null(L);

    ret = t_parse_simple(L);
    while (L->current->kind == T_AND) {
        and = L->current;
        (void)t_lex_next_token(L);
        ret = t_ast_new(ret, and, t_parse_simple(L));
    }

    return (ret);
}


static struct t_ast *
t_parse_simple(struct t_lexer *L)
{
    struct t_token *t;
    struct t_ast *ret;

    assert_not_null(L);

    t = L->current;
    switch (t->kind) {
    case T_NOT:
        ret = t_parse_not(L);
        break;
    case T_OPAREN:
        ret = t_parse_nestedcond(L);
        break;
    case T_INT:      /* FALLTHROUGH */
    case T_DOUBLE:   /* FALLTHROUGH */
    case T_STRING:   /* FALLTHROUGH */
    case T_REGEX:    /* FALLTHROUGH */
    case T_FILENAME: /* FALLTHROUGH */
    case T_UNDEF:    /* FALLTHROUGH */
    case T_BACKEND:  /* FALLTHROUGH */
    case T_TAGKEY:
        ret = parse_cmp_or_match_or_value(L);
        break;
    default:
        parse_error(L, t, t,"expected  NOT, OPAREN, REGEX or value, got %s",
                t->str);
        /* NOTREACHED */
    }

    return (ret);
}


static struct t_ast *
t_parse_not(struct t_lexer *L)
{
    struct t_token *not;
    struct t_ast *cond, *ret;

    assert_not_null(L);

    not = L->current;
    if (not->kind != T_NOT)
        parse_error(L, not, not, "expected NOT, got %s", not->str);

    (void)t_lex_next_token(L);
    cond = t_parse_nestedcond(L);
    ret = t_ast_new(NULL, not, cond);

    return (ret);
}


static struct t_ast *
t_parse_nestedcond(struct t_lexer *L)
{
    struct t_token *oparen, *cparen;
    struct t_ast *ret;

    assert_not_null(L);

    oparen = L->current;
    if (oparen->kind != T_OPAREN) {
        parse_error(L, oparen, oparen, "expected OPAREN, got %s",
                oparen->str);
        /* NOTREACHED */
    }

    (void)t_lex_next_token(L);
    ret = t_parse_condition(L);

    cparen = L->current;
    if (cparen->kind != T_CPAREN) {
        parse_error(L, oparen, cparen, "expected CPAREN, got %s",
                cparen->str);
        /* NOTREACHED */
    }
    (void)t_lex_next_token(L);

    ret->start = oparen->start;
    ret->end   = cparen->end;
    freex(oparen);
    freex(cparen);
    return (ret);
}


/*
 * 3) Condition ::= <Value> ( '==' | '<' | '<=' | '>' | '>=' | '!=' ) <Value>
 *    Condition ::= <Value> ( '=~' | '!~' ) <REGEX>
 *    Condition ::= <REGEX> ( '=~' | '!~' ) <Value>
 */
static struct t_ast *
parse_cmp_or_match_or_value(struct t_lexer *L)
{
    struct t_token *lhstok, *optok, *rhstok;

    assert_not_null(L);
    lhstok = L->current;
    switch (lhstok->kind) {
    case T_INT:      /* FALLTHROUGH */
    case T_DOUBLE:   /* FALLTHROUGH */
    case T_STRING:   /* FALLTHROUGH */
    case T_REGEX:    /* FALLTHROUGH */
    case T_FILENAME: /* FALLTHROUGH */
    case T_UNDEF:    /* FALLTHROUGH */
    case T_BACKEND: /* FALLTHROUGH */
    case T_TAGKEY:
        break;
    default:
        parse_error(L, lhstok, lhstok, "expected REGEX or value, got %s",
                lhstok->str);
        /* NOTREACHED */
    }

    optok = t_lex_next_token(L);
    switch (optok->kind) {
    case T_EQ:     /* FALLTHROUGH */
    case T_NE:     /* FALLTHROUGH */
    case T_MATCH:  /* FALLTHROUGH */
    case T_NMATCH: /* FALLTHROUGH */
    case T_LT:     /* FALLTHROUGH */
    case T_LE:     /* FALLTHROUGH */
    case T_GT:     /* FALLTHROUGH */
    case T_GE:     /* FALLTHROUGH */
        break;
    default:
        parse_error(L, lhstok, optok, "expected <value operator value>, got %s %s",
                lhstok->str, optok->str);
        /* NOTREACHED */
    }

    rhstok = t_lex_next_token(L);
    switch (rhstok->kind) {
    case T_INT:      /* FALLTHROUGH */
    case T_DOUBLE:   /* FALLTHROUGH */
    case T_STRING:   /* FALLTHROUGH */
    case T_REGEX:    /* FALLTHROUGH */
    case T_FILENAME: /* FALLTHROUGH */
    case T_UNDEF:    /* FALLTHROUGH */
    case T_BACKEND:  /* FALLTHROUGH */
    case T_TAGKEY:
        break;
    default:
        parse_error(L, rhstok, rhstok, "expected REGEX or value, got %s",
                rhstok->str);
        /* NOTREACHED */
    }

    if (optok->kind == T_MATCH || optok->kind == T_NMATCH) {
    /*
     * Condition ::= <Value> ( '=~' | '!~' ) <REGEX>
     * Condition ::= <REGEX> ( '=~' | '!~' ) <Value>
     */
        if ((lhstok->kind == T_REGEX && rhstok->kind == T_REGEX) ||
                (lhstok->kind != T_REGEX && rhstok->kind != T_REGEX)) {
            parse_error(L, lhstok, rhstok, "expected <REGEX (N)MATCH value> or"
                    " <value (N)MATCH REGEX>, got %s %s %s",
                    lhstok->str, optok->str, rhstok->str);
            /* NOTREACHED */
        }
    }
    else {
    /* Condition ::= <Value> ( '==' | '<' | '<=' | '>' | '>=' | '!=' ) <Value> */
        if (lhstok->kind == T_REGEX || rhstok->kind == T_REGEX) {
            parse_error(L, lhstok, rhstok, "unexpected REGEX (not a MATCH operator!),"
                    " got %s %s %s", lhstok->str, optok->str, rhstok->str);
            /* NOTREACHED */
        }
    }

    /* avoid constant evaluation */
    if (lhstok->kind != T_TAGKEY && lhstok->kind != T_FILENAME &&
            lhstok->kind != T_BACKEND && rhstok->kind != T_TAGKEY &&
            rhstok->kind != T_FILENAME && rhstok->kind != T_BACKEND) {
        parse_error(L, lhstok, rhstok, "constant comparison: %s %s %s",
                lhstok->str, optok->str, rhstok->str);
        /* NOTREACHED */
    }

    (void)t_lex_next_token(L);
    return (t_ast_new(t_ast_new(NULL, lhstok, NULL), optok, t_ast_new(NULL, rhstok, NULL)));
}


void
parse_error(const struct t_lexer *L,
        const struct t_token *startt, const struct t_token *endt,
        const char *fmt, ...)
{
    va_list args;

    assert_not_null(L);
    assert_not_null(startt);
    assert_not_null(endt);

    fprintf(stderr, "parser error: ");
    va_start(args, fmt);
        t_lex_error0(L, startt->start, endt->end, fmt, args);
    va_end(args);

    exit(EINVAL);
    /* NOTREACHED */
}


#if 0
void
t_ast_debug(struct t_ast *AST)
{

    if (AST == NULL)
        return;

    t_lex_token_debug(AST->token);
    if (AST->rhs) {
        (void)printf("[");
        if (AST->lhs) {
            t_ast_debug(AST->lhs);
            (void)printf(", ");
        }
        t_ast_debug(AST->rhs);
        (void)printf("]");
    }
}

int
main(int argc, char *argv[]) {
    struct t_ast *AST;

    if (argc < 2)
        errx(-1, "usage: %s [conditions]\n", argv[0]);

    AST = t_parse_filter(t_lexer_new(argv[1]));

    t_ast_debug(AST);
    (void)printf("\n");
    t_ast_destroy(AST);

    return (0);
}
#endif

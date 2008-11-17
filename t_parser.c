/*
 * t_parser.c
 *
 * a LL(1) recursive descend parser for tagutil.
 * used by the filter function.
 */
#include "t_config.h"

#include <string.h>

#include "t_lexer.h"
#include "t_parser.h"
#include "t_toolkit.h"


static inline struct ast * new_node(struct ast *restrict lhs,
        enum tokenkind tkind, struct ast *restrict rhs);

static inline struct ast * new_leaf(enum tokenkind tkind, void *value);

/* private parse_filter helpers. */
__t__nonnull(1)
static struct ast * parse_condition(struct lexer *restrict L);

/*
 * Condition ::= <Condition> '||' <Condition>
 */
__t__nonnull(1)
static struct ast * parse_or(struct lexer *restrict L);

/*
 * Condition ::= <Condition> '&&' <Condition>
 */
__t__nonnull(1)
static struct ast * parse_and(struct lexer *restrict L);

/*
 * choose which factory we need.
 */
__t__nonnull(1)
static struct ast * parse_cmp(struct lexer *restrict L);

/*
 * Condition ::= <IntKeyword> ( '==' | '<' | '<=' | '>' | '>=' | '!=' ) <INTEGER>
 */
__t__nonnull(1)
static struct ast * parse_intcmp(struct lexer *restrict L);

/*
 * Condition ::= <StrKeyword> ( '==' | '<' | '<=' | '>' | '>=' | '!=' | '=~' ) <STRING>
 * Condition ::= <StrKeyword> ( '==' | '<' | '<=' | '>' | '>=' | '!=' | '=~' ) <StrKeyword>
 * Condition ::= <StrKeyword> '=~' <REGEX>
 */
__t__nonnull(1)
static struct ast * parse_strcmp(struct lexer *restrict L);

/*
 * Condition ::= '!' '(' <Condition> ')'
 */
__t__nonnull(1)
static struct ast * parse_not(struct lexer *restrict L);

/*
 * Condition ::= '(' <Condition> ')'
 */
__t__nonnull(1)
static struct ast * parse_nestedcond(struct lexer *restrict L);


static inline struct ast *
new_node(struct ast *restrict lhs, enum tokenkind tkind,
        struct ast *restrict rhs)
{
    struct ast *ret;

    ret = xmalloc(sizeof(struct ast));

    ret->kind = ANODE;
    ret->lhs = lhs;
    ret->rhs = rhs;
    ret->tkind = tkind;

    return (ret);
}


static inline struct ast *
new_leaf(enum tokenkind tkind, void *value)
{
    struct ast *ret;

    ret = xmalloc(sizeof(struct ast));

    ret->kind  = ALEAF;
    ret->tkind = tkind;

    switch(tkind) {
    case TSTRING:
        ret->value.string = (char *)value;
        break;
    case TREGEX:
        memcpy(&ret->value.regex, value, sizeof(regex_t));
        break;
    case TINT:
        ret->value.integer = *(int *)value;
        break;
    case TTITLE:   /* FALLTHROUGH */
    case TALBUM:   /* FALLTHROUGH */
    case TARTIST:  /* FALLTHROUGH */
    case TYEAR:    /* FALLTHROUGH */
    case TTRACK:   /* FALLTHROUGH */
    case TCOMMENT: /* FALLTHROUGH */
    case TGENRE:   /* FALLTHROUGH */
    case TFILENAME:
        /* NOOP */
        break;
    default:
        errx(-1, "parser error, expected leaf.");
        /* NOTREACHED */
    }

    return (ret);
}


void
destroy_ast(struct ast *restrict victim)
{
    if (victim != NULL) {
        switch (victim->kind) {
        case ANODE:
            destroy_ast(victim->lhs);
            destroy_ast(victim->rhs);
            free(victim);
            break;
        case ALEAF:
            if (victim->tkind == TSTRING)
                free(victim->value.string);
            free(victim);
            break;
        default:
            errx(-1, "error in destroy_ast: AST without kind.");
            /* NOTREACHED */
        }
    }
}


struct ast *
parse_filter(struct lexer *restrict L)
{
    struct ast *ret;

    assert_not_null(L);

    ret = NULL;

    switch (L->current.kind) {
    case TSTART:
        lex(L);
        break;
    default:
        errx(-1, "parser error, expected TSTART to begin, got %s",
                token_to_s(L->current.kind));
        /* NOTREACHED */
    }

    ret = parse_condition(L);

    switch (L->current.kind) {
    case TEOS:
        lex(L);
        break;
    default:
        errx(-1, "parser error, expected TEOS at the end, got %s",
                token_to_s(L->current.kind));
        /* NOTREACHED */
    }

    free(L);
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
    struct ast *lhs;

    assert_not_null(L);

    lhs = parse_and(L);
    while (L->current.kind == TOR) {
        lex(L);
        lhs = new_node(lhs, TOR, parse_and(L));
    }

    return (lhs);
}


struct ast *
parse_and(struct lexer *restrict L)
{
    struct ast *lhs;

    assert_not_null(L);

    lhs = parse_cmp(L);
    while (L->current.kind == TAND) {
        lex(L);
        lhs = new_node(lhs, TAND, parse_cmp(L));
    }

    return (lhs);
}


static struct ast *
parse_cmp(struct lexer *restrict L)
{
    struct ast *ret;

    assert_not_null(L);

    ret = NULL;

    switch(L->current.kind) {
    case TYEAR: /* FALLTHROUGH */
    case TTRACK:
        ret = parse_intcmp(L);
        break;
    case TTITLE:   /* FALLTHROUGH */
    case TALBUM:   /* FALLTHROUGH */
    case TARTIST:  /* FALLTHROUGH */
    case TCOMMENT: /* FALLTHROUGH */
    case TGENRE:   /* FALLTHROUGH */
    case TFILENAME:
        ret = parse_strcmp(L);
        break;
    case TOPAREN:
        ret = parse_nestedcond(L);
        break;
    case TNOT:
        ret = parse_not(L);
        break;
    default:
        errx(-1, "parser error at %d-%d: expected keyword or TNOT or TOPAREN, got %s",
                L->current.start, L->current.end, token_to_s(L->current.kind));
        /* NOTREACHED */
    }

    return (ret);
}


static struct ast *
parse_intcmp(struct lexer *restrict L)
{
    struct ast *lhs, *ret;
    enum tokenkind tkind;

    ret = NULL;
    lhs = new_leaf(L->current.kind, NULL);

    lex(L);
    switch (L->current.kind) {
    case TEQ: /* FALLTHROUGH */
    case TLT: /* FALLTHROUGH */
    case TLE: /* FALLTHROUGH */
    case TGT: /* FALLTHROUGH */
    case TGE: /* FALLTHROUGH */
    case TDIFF:
        tkind = L->current.kind;
        break;
    default:
        errx(-1, "parser error at %d-%d: expected TEQ or TLT or TLE or TGL or TGE or TDIFF, got %s",
                L->current.start, L->current.end, token_to_s(L->current.kind));
        /* NOTREACHED */
    }

    lex(L);
    switch (L->current.kind) {
    case TINT:
        ret = new_node(lhs, tkind, new_leaf(TINT, &L->current.value.integer));
        lex(L);
        break;
    default:
        errx(-1, "parser error at %d-%d: expected TINT, got %s",
                L->current.start, L->current.end, token_to_s(L->current.kind));
        /* NOTREACHED */
    }

    return (ret);
}


static struct ast *
parse_strcmp(struct lexer *restrict L)
{
    struct ast *lhs, *ret;
    enum tokenkind tkind;
    bool regex;

    assert_not_null(L);

    regex = false;
    ret = NULL;
    lhs = new_leaf(L->current.kind, NULL);

    lex(L);
    switch (L->current.kind) {
    case TEQ: /* FALLTHROUGH */
    case TLT: /* FALLTHROUGH */
    case TLE: /* FALLTHROUGH */
    case TGT: /* FALLTHROUGH */
    case TGE: /* FALLTHROUGH */
    case TMATCH: /* FALLTHROUGH */
    case TDIFF:
        tkind = L->current.kind;
        break;
    default:
        errx(-1, "parser error at %d-%d: expected TEQ or TLT or TLE or TGL or TGE or TDIFF or TMATCH, got %s",
                L->current.start, L->current.end, token_to_s(L->current.kind));
        /* NOTREACHED */
    }

    lex(L);
    switch (L->current.kind) {
    case TTITLE:    /* FALLTHROUGH */
    case TALBUM:    /* FALLTHROUGH */
    case TARTIST:   /* FALLTHROUGH */
    case TCOMMENT:  /* FALLTHROUGH */
    case TGENRE:    /* FALLTHROUGH */
    case TFILENAME: /* FALLTHROUGH */
    case TSTRING:
        if (tkind == TMATCH) {
            errx(-1, "parser error at %d-%d: expected TREGEX, got %s",
                L->current.start, L->current.end, token_to_s(L->current.kind));
        }
        ret = new_node(lhs, tkind, new_leaf(L->current.kind,
                    L->current.kind == TSTRING ? L->current.value.string : NULL));
        break;
    case TREGEX:
        if (tkind != TMATCH) {
            errx(-1, "parser error at %d-%d: expected TSTRING, got %s",
                L->current.start, L->current.end, token_to_s(L->current.kind));
        }
        ret = new_node(lhs, tkind, new_leaf(TREGEX, &L->current.value.regex));
        break;
    default:
        errx(-1, "parser error at %d-%d: expected %s, got %s",
                L->current.start,
                L->current.end, tkind == TMATCH ? "TREGEX" : "TSTRING",
                token_to_s(L->current.kind));
        /* NOTREACHED */
    }

    lex(L);
    return (ret);
}


static struct ast *
parse_not(struct lexer *restrict L)
{
    struct ast *cond;

    assert_not_null(L);

    cond = NULL;

    switch (L->current.kind) {
    case TNOT:
        lex(L);
        break;
    default:
        errx(-1, "parser error at %d-%d: expected TNOT, got %s",
                L->current.start, L->current.end, token_to_s(L->current.kind));
        /* NOTREACHED */
    }

    cond = parse_nestedcond(L);

    return (new_node(NULL, TNOT, cond));
}


static struct ast *
parse_nestedcond(struct lexer *restrict L)
{
    struct ast *ret;

    assert_not_null(L);

    ret = NULL;

    switch (L->current.kind) {
    case TOPAREN:
        lex(L);
        break;
    default:
        errx(-1, "parser error at %d-%d: expected TOPAREN, got %s",
                L->current.start, L->current.end, token_to_s(L->current.kind));
        /* NOTREACHED */
    }

    ret = parse_condition(L);

    switch (L->current.kind) {
    case TCPAREN:
        lex(L);
        break;
    default:
        errx(-1, "parser error at %d-%d: expected TCPAREN, got %s",
                L->current.start, L->current.end, token_to_s(L->current.kind));
        /* NOTREACHED */
    }

    return (ret);
}


#if 0
void
astprint(struct ast *AST)
{

    if (AST == NULL)
        return;

    switch (AST->kind) {
    case ANODE:
        (void)printf("(");
        astprint(AST->lhs);
        (void)printf(" ");
        (void)printf("%s", token_to_s(AST->tkind));
        (void)printf(" ");
        astprint(AST->rhs);
        (void)printf(")");
        break;
    default:
        (void)printf("%s", token_to_s(AST->tkind));
        switch (AST->tkind) {
        case TSTRING:
            (void)printf("(%s)", AST->value.string);
            break;
        case TINT:
            (void)printf("(%d)", AST->value.integer);
            break;
        default:
            break;
        }
        break;
    }
}

int
main(int argc, char *argv[]) {
    struct ast *AST;
    struct lexer *L;

    if (argc < 2)
        errx(-1, "usage: %s [conditions]\n", argv[0]);

    L = new_lexer(argv[1]);
    AST = parse_filter(L);

    astprint(AST);
    (void)printf("\n");

    return (0);
}
#endif

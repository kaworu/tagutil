/*
 * t_interpreter.c
 *
 * the tagutil's filter interpreter.
 */
#include "t_config.h"

#include "t_toolkit.h"
#include "t_lexer.h"
#include "t_parser.h"
#include "t_interpreter.h"


_t__nonnull(1) _t__nonnull(2)
static inline unsigned int int_leaf(const struct tfile *restrict file,
        struct ast *restrict leaf);

_t__nonnull(1) _t__nonnull(2)
static inline const char * str_leaf(const struct tfile *restrict file,
const struct ast *restrict leaf);


bool
eval(const struct tfile *restrict file, const struct ast *restrict filter)
#define EVAL(e) (eval(file, (e)))
#define ILEAF(k) (int_leaf(file, (k)))
#define SLEAF(k) (str_leaf(file, (k)))
{
    bool ret;
    char s[BUFSIZ];
    int error;

    assert_not_null(file);
    assert_not_null(filter);

    if (filter->kind == ALEAF)
        errx(-1, "can't eval a leaf AST");

    switch (filter->tkind) {
    case TOR:
        ret = (EVAL(filter->lhs) || EVAL(filter->rhs));
        break;
    case TAND:
        ret = (EVAL(filter->lhs) && EVAL(filter->rhs));
        break;
    case TNOT:
        ret = (!EVAL(filter->rhs));
        break;
    case TEQ:
        if (is_int_tkeyword(filter->lhs->tkind))
            ret = (ILEAF(filter->lhs) == ILEAF(filter->rhs));
        else
            ret = (strcmp(SLEAF(filter->lhs), SLEAF(filter->rhs)) == 0);
        break;
    case TLT:
        if (is_int_tkeyword(filter->lhs->tkind))
            ret = (ILEAF(filter->lhs) < ILEAF(filter->rhs));
        else
            ret = (strcmp(SLEAF(filter->lhs), SLEAF(filter->rhs)) < 0);
        break;
    case TLE:
        if (is_int_tkeyword(filter->lhs->tkind))
            ret = (ILEAF(filter->lhs) <= ILEAF(filter->rhs));
        else
            ret = (strcmp(SLEAF(filter->lhs), SLEAF(filter->rhs)) <= 0);
        break;
    case TGT:
        if (is_int_tkeyword(filter->lhs->tkind))
            ret = (ILEAF(filter->lhs) > ILEAF(filter->rhs));
        else
            ret = (strcmp(SLEAF(filter->lhs), SLEAF(filter->rhs)) > 0);
        break;
    case TGE:
        if (is_int_tkeyword(filter->lhs->tkind))
            ret = (ILEAF(filter->lhs) >= ILEAF(filter->rhs));
        else
            ret = (strcmp(SLEAF(filter->lhs), SLEAF(filter->rhs)) >= 0);
        break;
    case TDIFF:
        if (is_int_tkeyword(filter->lhs->tkind))
            ret = (ILEAF(filter->lhs) != ILEAF(filter->rhs));
        else
            ret = (strcmp(SLEAF(filter->lhs), SLEAF(filter->rhs)) != 0);
        break;
    case TMATCH:
        error = regexec(&filter->rhs->value.regex, SLEAF(filter->lhs), 1, NULL, 0);

        switch (error) {
        case 0:
            ret = true;
            break;
        case REG_NOMATCH:
            ret = false;
            break;
        default:
            (void)regerror(error, &filter->rhs->value.regex, s, sizeof(s));
            errx(-1, "interpreter error, can't exec regex: '%s'", s);
            /* NOTREACHED */
        }
        break;
    default:
        errx(-1, "interpreter error: unexpected token: '%s'", token_to_s(filter->tkind));
        /* NOTREACHED */
    }

    return (ret);
}

static inline unsigned int
int_leaf(const struct tfile *restrict file, struct ast *restrict leaf)
{
    unsigned int ret;

    assert_not_null(file);
    assert_not_null(leaf);
    assert(leaf->kind == ALEAF);

    switch (leaf->tkind) {
    case TTRACK:
        ret = atoi(file->get(file, "track")); /* FIXME: atoi(3) sucks */
        break;
    case TYEAR:
        ret = atoi(file->get(file, "year")); /* FIXME: atoi(3) sucks */
        break;
    case TINT:
        ret = leaf->value.integer;
        break;
    default:
        errx(-1, "intepreter error: expected TTRACK or TYEAR or TINT, got %s",
                token_to_s(leaf->tkind));
        /* NOTREACHED */
    }

    return (ret);
}

static inline const char *
str_leaf(const struct tfile *restrict file, const struct ast *restrict leaf)
{
    const char *ret;

    assert_not_null(file);
    assert_not_null(leaf);
    assert(leaf->kind == ALEAF);

    switch (leaf->tkind) {
    case TTITLE:
        ret = file->get(file, "title");
        break;
    case TALBUM:
        ret = file->get(file, "album");
        break;
    case TARTIST:
        ret = file->get(file, "artist");
        break;
    case TCOMMENT:
        ret = file->get(file, "comment");
        break;
    case TGENRE:
        ret = file->get(file, "genre");
        break;
    case TFILENAME:
        ret = file->path;
        break;
    case TSTRING:
        ret = leaf->value.string;
        break;
    default:
        errx(-1, "intepreter error: str_tag: expected TTITLE or TALBUM or TARTIST"
                 " or TCOMMENT or TGENRE or TFILENAME, got %s", token_to_s(leaf->tkind));
        /* NOTREACHED */
    }

    return (ret);
}


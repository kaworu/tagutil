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


__t__nonnull(1) __t__nonnull(2)
static inline unsigned int int_leaf(struct ast *restrict leaf,
        const TagLib_Tag *restrict tag);

__t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
static inline const char * str_leaf(const char *restrict filename,
        const struct ast *restrict leaf, const TagLib_Tag *restrict tag);


bool
eval(const char *restrict filename, const TagLib_Tag *restrict tag,
        const struct ast *restrict filter)
#define EVAL(e) (eval(filename, tag, (e)))
#define ILEAF(k) (int_leaf((k), tag))
#define SLEAF(k) (str_leaf(filename, (k), tag))
{
    bool ret;
    char *s;
    int error;

    assert_not_null(filename);
    assert_not_null(tag);
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
            s = xcalloc(BUFSIZ, sizeof(char));
            (void)regerror(error, &filter->rhs->value.regex, s, BUFSIZ);
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
int_leaf(struct ast *restrict leaf, const TagLib_Tag *restrict tag)
{
    unsigned int ret;

    assert_not_null(leaf);
    assert(leaf->kind == ALEAF);
    assert_not_null(tag);

    switch (leaf->tkind) {
    case TTRACK:
        ret = taglib_tag_track(tag);
        break;
    case TYEAR:
        ret = taglib_tag_year(tag);
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
str_leaf(const char *restrict filename, const struct ast *restrict leaf,
        const TagLib_Tag *restrict tag)
{
    const char *ret;

    assert_not_null(filename);
    assert_not_null(leaf);
    assert(leaf->kind == ALEAF);
    assert_not_null(tag);

    switch (leaf->tkind) {
    case TTITLE:
        ret = taglib_tag_title(tag);
        break;
    case TALBUM:
        ret = taglib_tag_album(tag);
        break;
    case TARTIST:
        ret = taglib_tag_artist(tag);
        break;
    case TCOMMENT:
        ret = taglib_tag_comment(tag);
        break;
    case TGENRE:
        ret = taglib_tag_genre(tag);
        break;
    case TFILENAME:
        ret = filename;
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


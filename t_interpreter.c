/*
 * t_interpreter.c
 *
 * the tagutil's filter interpreter.
 */

#include "config.h"
#include "t_toolkit.h"
#include "t_lexer.h"
#include "t_parser.h"
#include "t_interpreter.h"

static unsigned int int_tag(enum tokenkind tkind, const TagLib_Tag *restrict tag);
static const char * str_tag(const char *restrict filename,
        enum tokenkind tkind, const TagLib_Tag *restrict tag);

bool
eval(const char *restrict filename,
        const TagLib_Tag *restrict tag,
        const struct ast *restrict filter)
#define EVAL(e) (eval(filename, tag, (e)))
#define INT_TAG(k) (int_tag((k), tag))
#define STR_TAG(k) (str_tag(filename, (k), tag))
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
            ret = (INT_TAG(filter->lhs->tkind) == filter->rhs->value.integer);
        else
            ret = (strcmp(STR_TAG(filter->lhs->tkind), filter->rhs->value.string) == 0);
        break;
    case TLT:
        if (is_int_tkeyword(filter->lhs->tkind))
            ret = (INT_TAG(filter->lhs->tkind) < filter->rhs->value.integer);
        else
            ret = (strcmp(STR_TAG(filter->lhs->tkind), filter->rhs->value.string) < 0);
        break;
    case TLE:
        if (is_int_tkeyword(filter->lhs->tkind))
            ret = (INT_TAG(filter->lhs->tkind) <= filter->rhs->value.integer);
        else
            ret = (strcmp(STR_TAG(filter->lhs->tkind), filter->rhs->value.string) <= 0);
        break;
    case TGT:
        if (is_int_tkeyword(filter->lhs->tkind))
            ret = (INT_TAG(filter->lhs->tkind) > filter->rhs->value.integer);
        else
            ret = (strcmp(STR_TAG(filter->lhs->tkind), filter->rhs->value.string) > 0);
        break;
    case TGE:
        if (is_int_tkeyword(filter->lhs->tkind))
            ret = (INT_TAG(filter->lhs->tkind) >= filter->rhs->value.integer);
        else
            ret = (strcmp(STR_TAG(filter->lhs->tkind), filter->rhs->value.string) >= 0);
        break;
    case TDIFF:
        if (is_int_tkeyword(filter->lhs->tkind))
            ret = (INT_TAG(filter->lhs->tkind) != filter->rhs->value.integer);
        else
            ret = (strcmp(STR_TAG(filter->lhs->tkind), filter->rhs->value.string) != 0);
        break;
    case TMATCH:
        error = regexec(&filter->rhs->value.regex, STR_TAG(filter->lhs->tkind), 1, NULL, 0);

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
            errx(-1, "interpreter error, can't exec regex: %s", s);
            /* NOTREACHED */
        }
        break;
    default:
        errx(-1, "interpreter error: unexpected token: %s", token_to_s(filter->tkind));
        /* NOTREACHED */
    }

    return (ret);
}

static unsigned int
int_tag(enum tokenkind tkind, const TagLib_Tag *restrict tag)
{
    unsigned int ret;

    switch (tkind) {
    case TTRACK:
        ret = taglib_tag_track(tag);
        break;
    case TYEAR:
        ret = taglib_tag_year(tag);
        break;
    default:
        errx(-1, "intepreter error: int_tag: expected TTRACK or TYEAR, got %s",
                token_to_s(tkind));
        /* NOTREACHED */
    }

    return (ret);
}

static const char *
str_tag(const char *restrict filename,
        enum tokenkind tkind,
        const TagLib_Tag *restrict tag)
{
    const char *ret;

    switch (tkind) {
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
    default:
        errx(-1, "intepreter error: str_tag: expected TTITLE or TALBUM or TARTIST"
                 " or TCOMMENT or TGENRE or TFILENAME, got %s", token_to_s(tkind));
        /* NOTREACHED */
    }

    return (ret);
}

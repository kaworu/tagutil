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


_t__nonnull(1) _t__nonnull(2) _t__nonnull(3)
static double eval_cmp(const struct tfile *restrict file,
        struct ast *restrict lhs, struct ast *restrict rhs, bool *undef);

_t__nonnull(1) _t__nonnull(2) _t__nonnull(3)
static bool eval_match(const struct tfile *restrict file,
        struct ast *restrict lhs, struct ast *restrict rhs, bool *undef);


bool
ast_eval(const struct tfile *restrict file, const struct ast *restrict filter)
{
    bool ret, undef;

    assert_not_null(file);
    assert_not_null(filter);

    undef = false;
    switch (filter->token->kind) {
    case TNOT:
        ret = !(ast_eval(file, filter->rhs));
        break;
    case TAND:
        ret = ast_eval(file, filter->lhs);
        if (ret)
            ret = ast_eval(file, filter->rhs);
        break;
    case TOR:
        ret = ast_eval(file, filter->lhs);
        if (!ret)
            ret = ast_eval(file, filter->rhs);
        break;
    case TEQ:
        ret = (eval_cmp(file, filter->lhs, filter->rhs, &undef) == 0);
        break;
    case TDIFF:
        ret = (eval_cmp(file, filter->lhs, filter->rhs, &undef) != 0);
        break;
    case TLT:
        ret = (eval_cmp(file, filter->lhs, filter->rhs, &undef) <  0);
        break;
    case TLE:
        ret = (eval_cmp(file, filter->lhs, filter->rhs, &undef) <= 0);
        break;
    case TGT:
        ret = (eval_cmp(file, filter->lhs, filter->rhs, &undef) >  0);
        break;
    case TGE:
        ret = (eval_cmp(file, filter->lhs, filter->rhs, &undef) >= 0);
        break;
    case TMATCH:
        ret = eval_match(file, filter->lhs, filter->rhs, &undef);
        break;
    case TNMATCH:
        ret = !eval_match(file, filter->lhs, filter->rhs, &undef);
        break;
    default:
        fprintf(stderr, "internal interpreter error: unexpected AST: `%s'",
                filter->token->str);
        abort();
        /* NOTREACHED */
    }

    if (undef)
        ret = false;

    return (ret);
}


static double
eval_cmp(const struct tfile *restrict file,
        struct ast *restrict lhs, struct ast *restrict rhs, bool *_undef)
{
    const char *s, *l, *r;
    char *_s, *_l, *_r; /* not const, can be free()d */
    bool die, *undef;
    double ret;

    assert_not_null(file);
    assert_not_null(rhs);
    assert_not_null(lhs);

    if (_undef == NULL)
        undef = &die;
    else
        undef = _undef;
    *undef = false;

    _s = _l = _r = NULL;
    switch (rhs->token->kind) {
    case TINT:    /* FALLTHROUGH */
    case TDOUBLE: /* FALLTHROUGH */
    case TSTRING:
        if (lhs->token->kind == TFILENAME)
            s = file->path;
        else if (lhs->token->kind == TBACKEND)
            s = file->lib;
        else {
            assert(lhs->token->kind == TTAGKEY);
            s = _s = file->get(file, lhs->token->value.str);
            if (s == NULL) {
                *undef = true;
                return (0);
            }
        }
        if (rhs->token->kind == TINT)
            ret = (double)(strtol(s, NULL, 10) - rhs->token->value.integer);
        else if (rhs->token->kind == TDOUBLE)
            ret = strtod(s, NULL) - rhs->token->value.dbl;
        else if (rhs->token->kind == TSTRING)
            ret = (double)strcmp(s, rhs->token->value.str);
        xfree(_s);
        break;
    case TUNDEF:
        if (lhs->token->kind == TFILENAME || lhs->token->kind == TBACKEND)
            ret = 1.0;
        else {
            assert(lhs->token->kind == TTAGKEY);
            s = _s = file->get(file, lhs->token->value.str);
            if (s == NULL)
                ret = 0.0;
            else
                ret = 1.0;
        }
        xfree(_s);
        break;
    case TFILENAME: /* FALLTHROUGH */
    case TTAGKEY:
        if (lhs->token->kind == TFILENAME || lhs->token->kind == TTAGKEY ||
                lhs->token->kind == TBACKEND) {
            if (lhs->token->kind == TFILENAME)
                l = file->path;
            else if (lhs->token->kind == TBACKEND)
                l = file->lib;
            else
                l = _l = file->get(file, lhs->token->value.str);
            if (rhs->token->kind == TFILENAME)
                r = file->path;
            else if (lhs->token->kind == TBACKEND)
                r = file->lib;
            else
                r = _r = file->get(file, rhs->token->value.str);
            if (r == NULL || l == NULL) {
                xfree(_l);
                xfree(_r);
                *undef = true;
                return (0);
            }
            /* XXX: cast? */
            ret = (double)strcmp(l, r);
            xfree(_l);
            xfree(_r);
        }
        else {
            ret = eval_cmp(file, rhs, lhs, undef);
            ret = -ret;
        }
        break;
    default:
        fprintf(stderr, "internal interpreter error: unexpected token kind:"
                " lhs(%s) rhs(%s)", lhs->token->str, rhs->token->str);
        abort();
        /* NOTREACHED */
    }

    return (ret);
}

static bool
eval_match(const struct tfile *restrict file,
        struct ast *restrict lhs, struct ast *restrict rhs, bool *_undef)
{
    regex_t *r;
    int error;
    bool die, ret, *undef;
    const char *s;
    char *_s = NULL;
    char errmsg[BUFSIZ];
    struct ast *regast, *strast;

    assert_not_null(file);
    assert_not_null(rhs);
    assert_not_null(lhs);

    if (_undef == NULL)
        undef = &die;
    else
        undef = _undef;
    *undef = false;

    if (rhs->token->kind == TREGEX) {
        regast = rhs;
        strast = lhs;
    }
    else {
        regast = lhs;
        strast = rhs;
    }

    assert(regast->token->kind == TREGEX);
    r = &regast->token->value.regex;
    switch (strast->token->kind) {
    case TTAGKEY:
        s = _s = file->get(file, strast->token->value.str);
        if (s == NULL) {
            *undef = true;
            return (false);
        }
        break;
    case TFILENAME:
        s = file->path;
        break;
    case TBACKEND:
        s = file->lib;
        break;
    default:
        fprintf(stderr, "internal interpreter error: unexpected AST: `%s'\n",
                strast->token->str);
        abort();
        /* NOTREACHED */
    }

    error = regexec(r, s, /* nmatch */1, /* captures */NULL, /* eflags */0);
    switch (error) {
        case 0:
            ret = true;
            break;
        case REG_NOMATCH:
            ret = false;
            break;
        default:
            /* FIXME */
            (void)regerror(error, r, errmsg, sizeof(errmsg));
            errx(-1, "interpreter error, can't exec regex: '%s'", errmsg);
            /* NOTREACHED */
    }

    xfree(_s);
    return (ret);
}

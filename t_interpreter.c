/*
 * t_interpreter.c
 *
 * the tagutil's filter interpreter.
 */
#include <err.h>
#include <limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "t_config.h"
#include "t_file.h"
#include "t_lexer.h"
#include "t_parser.h"
#include "t_interpreter.h"


_t__nonnull(1) _t__nonnull(2) _t__nonnull(3)
static double t_interpreter_eval_cmp(const struct t_file *restrict file,
        struct t_ast *restrict lhs, struct t_ast *restrict rhs, bool *undef);

_t__nonnull(1) _t__nonnull(2) _t__nonnull(3)
static bool t_interpreter_eval_match(const struct t_file *restrict file,
        struct t_ast *restrict lhs, struct t_ast *restrict rhs, bool *undef);


bool
t_interpreter_eval_ast(const struct t_file *restrict file,
        const struct t_ast *restrict filter)
{
    bool ret, undef;

    assert_not_null(file);
    assert_not_null(filter);

    undef = false;
    switch (filter->token->kind) {
    case T_NOT:
        ret = !(t_interpreter_eval_ast(file, filter->rhs));
        break;
    case T_AND:
        ret = t_interpreter_eval_ast(file, filter->lhs);
        if (ret)
            ret = t_interpreter_eval_ast(file, filter->rhs);
        break;
    case T_OR:
        ret = t_interpreter_eval_ast(file, filter->lhs);
        if (!ret)
            ret = t_interpreter_eval_ast(file, filter->rhs);
        break;
    case T_EQ:
        ret = (t_interpreter_eval_cmp(file, filter->lhs, filter->rhs, &undef) == 0);
        break;
    case T_DIFF:
        ret = (t_interpreter_eval_cmp(file, filter->lhs, filter->rhs, &undef) != 0);
        break;
    case T_LT:
        ret = (t_interpreter_eval_cmp(file, filter->lhs, filter->rhs, &undef) <  0);
        break;
    case T_LE:
        ret = (t_interpreter_eval_cmp(file, filter->lhs, filter->rhs, &undef) <= 0);
        break;
    case T_GT:
        ret = (t_interpreter_eval_cmp(file, filter->lhs, filter->rhs, &undef) >  0);
        break;
    case T_GE:
        ret = (t_interpreter_eval_cmp(file, filter->lhs, filter->rhs, &undef) >= 0);
        break;
    case T_MATCH:
        ret = t_interpreter_eval_match(file, filter->lhs, filter->rhs, &undef);
        break;
    case T_NMATCH:
        ret = !t_interpreter_eval_match(file, filter->lhs, filter->rhs, &undef);
        break;
    default:
        (void)fprintf(stderr, "***internal interpreter error*** "
                "unexpected AST: `%s'\n", filter->token->str);
        assert_fail();
        /* NOTREACHED */
    }

    if (undef)
        ret = false;

    return (ret);
}


static double
t_interpreter_eval_cmp(const struct t_file *restrict file,
        struct t_ast *restrict lhs, struct t_ast *restrict rhs, bool *_undef)
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
    case T_INT:    /* FALLTHROUGH */
    case T_DOUBLE: /* FALLTHROUGH */
    case T_STRING:
        if (lhs->token->kind == T_FILENAME)
            s = file->path;
        else if (lhs->token->kind == T_BACKEND)
            s = file->lib;
        else {
            assert(lhs->token->kind == T_TAGKEY);
            s = _s = file->get(file, lhs->token->value.str);
            if (s == NULL) {
                *undef = true;
                return (0);
            }
        }
        if (rhs->token->kind == T_INT)
            ret = (double)(strtol(s, NULL, 10) - rhs->token->value.integer);
        else if (rhs->token->kind == T_DOUBLE)
            ret = strtod(s, NULL) - rhs->token->value.dbl;
        else if (rhs->token->kind == T_STRING)
            ret = (double)strcmp(s, rhs->token->value.str);
        freex(_s);
        break;
    case T_UNDEF:
        if (lhs->token->kind == T_FILENAME || lhs->token->kind == T_BACKEND)
            ret = 1.0;
        else {
            assert(lhs->token->kind == T_TAGKEY);
            s = _s = file->get(file, lhs->token->value.str);
            if (s == NULL)
                ret = 0.0;
            else
                ret = 1.0;
        }
        freex(_s);
        break;
    case T_FILENAME: /* FALLTHROUGH */
    case T_TAGKEY:
        if (lhs->token->kind == T_FILENAME || lhs->token->kind == T_TAGKEY ||
                lhs->token->kind == T_BACKEND) {
            if (lhs->token->kind == T_FILENAME)
                l = file->path;
            else if (lhs->token->kind == T_BACKEND)
                l = file->lib;
            else
                l = _l = file->get(file, lhs->token->value.str);
            if (rhs->token->kind == T_FILENAME)
                r = file->path;
            else if (lhs->token->kind == T_BACKEND)
                r = file->lib;
            else
                r = _r = file->get(file, rhs->token->value.str);
            if (r == NULL || l == NULL) {
                freex(_l);
                freex(_r);
                *undef = true;
                return (0);
            }
            /* XXX: cast? */
            ret = (double)strcmp(l, r);
            freex(_l);
            freex(_r);
        }
        else {
            ret = t_interpreter_eval_cmp(file, rhs, lhs, undef);
            ret = -ret;
        }
        break;
    default:
        (void)fprintf(stderr, "***internal interpreter error*** "
                "unexpected token kind: lhs(%s) rhs(%s)\n",
                lhs->token->str, rhs->token->str);
        assert_fail();
        /* NOTREACHED */
    }

    return (ret);
}

static bool
t_interpreter_eval_match(const struct t_file *restrict file,
        struct t_ast *restrict lhs, struct t_ast *restrict rhs, bool *_undef)
{
    regex_t *r;
    int error;
    bool die, ret, *undef;
    const char *s;
    char *_s = NULL;
    char *errmsg;
    struct t_ast *regast, *strast;

    assert_not_null(file);
    assert_not_null(rhs);
    assert_not_null(lhs);

    if (_undef == NULL)
        undef = &die;
    else
        undef = _undef;
    *undef = false;

    if (rhs->token->kind == T_REGEX) {
        regast = rhs;
        strast = lhs;
    }
    else {
        regast = lhs;
        strast = rhs;
    }

    assert(regast->token->kind == T_REGEX);
    r = &regast->token->value.regex;
    switch (strast->token->kind) {
    case T_TAGKEY:
        s = _s = file->get(file, strast->token->value.str);
        if (s == NULL) {
            *undef = true;
            return (false);
        }
        break;
    case T_FILENAME:
        s = file->path;
        break;
    case T_BACKEND:
        s = file->lib;
        break;
    default:
        fprintf(stderr, "***internal interpreter error*** unexpected AST: `%s'\n",
                strast->token->str);
        assert_fail();
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
            errmsg = xcalloc(BUFSIZ, sizeof(char));
            (void)regerror(error, r, errmsg, BUFSIZ);
            errx(-1, "interpreter error, can't exec regex: `%s'", errmsg);
            /* NOTREACHED */
    }

    freex(_s);
    return (ret);
}

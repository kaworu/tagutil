/*
 * t_filter.c
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
#include "t_filter.h"


/*
 * compare arg to zero.
 */
typedef bool t_cmp_func(double);

static t_cmp_func t_cmp_eq;
static t_cmp_func t_cmp_ne;
static t_cmp_func t_cmp_lt;
static t_cmp_func t_cmp_le;
static t_cmp_func t_cmp_gt;
static t_cmp_func t_cmp_ge;

/*
 * return the invert function (if you switch rhs and lhs, you might need to
 * change the operator).
 */
_t__nonnull(1)
t_cmp_func * t_invert(t_cmp_func *f);

/*
 * eval a comparison between lhs and rhs.
 */
_t__nonnull(1) _t__nonnull(2) _t__nonnull(3) _t__nonnull(4)
static bool t_filter_eval_cmp(struct t_file *file,
        const struct t_ast *lhs, const struct t_ast *rhs,
        t_cmp_func *f);

_t__nonnull(1) _t__nonnull(3) _t__nonnull(4)
static bool t_filter_eval_int_cmp(struct t_file *file,
        int i, const struct t_ast *rhs, t_cmp_func *f);

_t__nonnull(1) _t__nonnull(3) _t__nonnull(4)
static bool t_filter_eval_double_cmp(struct t_file *file,
        double d, const struct t_ast *rhs, t_cmp_func *f);

_t__nonnull(1) _t__nonnull(2) _t__nonnull(3) _t__nonnull(4)
static bool t_filter_eval_str_cmp(struct t_file *file,
        const char *str, const struct t_ast *rhs,
        t_cmp_func *f);

_t__nonnull(1) _t__nonnull(2) _t__nonnull(3)
static bool t_filter_eval_undef_cmp(struct t_file *file,
        const struct t_ast *rhs, t_cmp_func *f);

/*
 * eval a regex match.
 */
_t__nonnull(1) _t__nonnull(2) _t__nonnull(3)
static bool t_filter_eval_match(struct t_file *file,
        const struct t_ast *lhs, const struct t_ast *rhs);

_t__nonnull(1) _t__nonnull(2)
static inline bool t_filter_regexec(const regex_t *r, const char *s);


bool
t_filter_eval(struct t_file *file,
        const struct t_ast *filter)
{
    t_cmp_func *f;
    bool ret;

    assert_not_null(file);
    assert_not_null(filter);

    switch (filter->token->kind) {
    case T_NOT:
        ret = !(t_filter_eval(file, filter->rhs));
        break;
    case T_AND:
        ret = t_filter_eval(file, filter->lhs);
        if (ret)
            ret = t_filter_eval(file, filter->rhs);
        break;
    case T_OR:
        ret = t_filter_eval(file, filter->lhs);
        if (!ret)
            ret = t_filter_eval(file, filter->rhs);
        break;
    case T_EQ: /* FALLTHROUGH */
    case T_NE: /* FALLTHROUGH */
    case T_LT: /* FALLTHROUGH */
    case T_LE: /* FALLTHROUGH */
    case T_GT: /* FALLTHROUGH */
    case T_GE:
        switch (filter->token->kind) {
        case T_EQ: f = t_cmp_eq; break;
        case T_NE: f = t_cmp_ne; break;
        case T_LT: f = t_cmp_lt; break;
        case T_LE: f = t_cmp_le; break;
        case T_GT: f = t_cmp_gt; break;
        case T_GE: f = t_cmp_ge; break;
        default: /* NOTREACHED */ assert_fail();
        }
        ret = t_filter_eval_cmp(file, filter->lhs, filter->rhs, f);
        break;
    case T_MATCH:
        ret = t_filter_eval_match(file, filter->lhs, filter->rhs);
        break;
    case T_NMATCH:
        ret = !t_filter_eval_match(file, filter->lhs, filter->rhs);
        break;
    default:
        (void)fprintf(stderr, "***internal filter error*** "
                "unexpected AST: `%s'\n", filter->token->str);
        assert_fail();
        /* NOTREACHED */
    }

    return (ret);
}


static bool
t_filter_eval_cmp(struct t_file *file,
        const struct t_ast *lhs, const struct t_ast *rhs,
        t_cmp_func *f)
{
    bool ret = false;
    struct t_taglist *T = NULL;
    struct t_tag *t;

    assert_not_null(file);
    assert_not_null(rhs);
    assert_not_null(lhs);
    assert_not_null(f);

    switch (lhs->token->kind) {
    case T_INT:
        ret = t_filter_eval_int_cmp(file, lhs->token->val.integer, rhs, f);
        break;
    case T_DOUBLE:
        ret = t_filter_eval_double_cmp(file, lhs->token->val.dbl, rhs, f);
        break;
    case T_STRING:
        ret = t_filter_eval_str_cmp(file, lhs->token->val.str, rhs, f);
        break;
    case T_UNDEF:
        ret = t_filter_eval_undef_cmp(file, rhs, f);
        break;
    case T_FILENAME:
        ret = t_filter_eval_str_cmp(file, file->path, rhs, f);
        break;
    case T_BACKEND:
        ret = t_filter_eval_str_cmp(file, file->libid, rhs, f);
        break;
    case T_TAGKEY:
        if (rhs->token->kind == T_TAGKEY) {
            T = file->get(file, lhs->token->val.str);
            if (T == NULL) {
                warnx("t_filter: `%s': %s",
                        file->path, t_error_msg(file));
                break;
            }
            if (T->count == 0)
                break;
            if (lhs->token->tidx == T_TOKEN_STAR) {
                char *s;
                switch (lhs->token->tidx_mod) {
                case T_TOKEN_STAR_NO_MOD:
                    s = t_taglist_join(T, " - ");
                    ret = t_filter_eval_str_cmp(file, s, rhs, f);
                    freex(s);
                    break;
                case T_TOKEN_STAR_OR_MOD:
                    ret = false;
                    TAILQ_FOREACH(t, T->tags, entries) {
                        if (t_filter_eval_str_cmp(file, t->val, rhs, f)) {
                            ret = true;
                            break;
                        }
                    }
                    break;
                case T_TOKEN_STAR_AND_MOD:
                    ret = true;
                    TAILQ_FOREACH(t, T->tags, entries) {
                        if (!t_filter_eval_str_cmp(file, t->val, rhs, f)) {
                            ret = false;
                            break;
                        }
                    }
                    break;
                }
            }
            else {
                t = t_taglist_tag_at(T, lhs->token->tidx);
                if (t != NULL)
                    ret = t_filter_eval_str_cmp(file, t->val, rhs, f);
            }
        }
        else
            ret = t_filter_eval_cmp(file, rhs, lhs, t_invert(f));
        break;
    default:
        (void)fprintf(stderr, "***internal filter error*** "
                "unexpected token kind: lhs(%s) rhs(%s)\n",
                lhs->token->str, rhs->token->str);
        assert_fail();
        /* NOTREACHED */
    }

    t_taglist_delete(T);
    return (ret);
}


static bool
t_filter_eval_match(struct t_file *file,
        const struct t_ast *lhs, const struct t_ast *rhs)
{
    const regex_t *r;
    bool ret = false;
    const char *cs;
    const struct t_ast *regast, *strast;
    struct t_taglist *T = NULL;
    struct t_tag  *t;

    assert_not_null(file);
    assert_not_null(rhs);
    assert_not_null(lhs);

    if (rhs->token->kind == T_REGEX) {
        regast = rhs;
        strast = lhs;
    }
    else {
        regast = lhs;
        strast = rhs;
    }
    assert(regast->token->kind == T_REGEX);

    r = &regast->token->val.regex;
    switch (strast->token->kind) {
    case T_TAGKEY:
        T = file->get(file, strast->token->val.str);
        if (T == NULL) {
            warnx("t_filter: `%s': %s",
                    file->path, t_error_msg(file));
            break;
        }
        if (T->count == 0)
            break;
        if (strast->token->tidx == T_TOKEN_STAR) {
            char *s;
            switch (strast->token->tidx_mod) {
            case T_TOKEN_STAR_NO_MOD:
                s = t_taglist_join(T, " - ");
                ret = t_filter_regexec(r, s);
                freex(s);
                break;
            case T_TOKEN_STAR_OR_MOD:
                ret = false;
                TAILQ_FOREACH(t, T->tags, entries) {
                    if (t_filter_regexec(r, t->val))
                        ret = true;
                        break;
                    }
                break;
            case T_TOKEN_STAR_AND_MOD:
                ret = true;
                TAILQ_FOREACH(t, T->tags, entries) {
                    if (!t_filter_regexec(r, t->val))
                        ret = false;
                        break;
                    }
                break;
            }
        }
        else {
            t = t_taglist_tag_at(T, strast->token->tidx);
            if (t != NULL)
                ret = t_filter_regexec(r, t->val);
        }
        break;
    case T_BACKEND: /* FALLTHROUGH */
    case T_FILENAME:
        if (strast->token->kind == T_FILENAME)
            cs = file->path;
        else
            cs = file->libid;
        ret = t_filter_regexec(r, cs);
        break;
    default:
        fprintf(stderr, "***internal filter error*** unexpected AST: `%s'\n",
                strast->token->str);
        assert_fail();
        /* NOTREACHED */
    }

    t_taglist_delete(T);
    return (ret);
}


static inline bool
t_filter_regexec(const regex_t *r, const char *s)
{
    int error;
    bool ret;
    char *errmsg;

    assert_not_null(s);
    assert_not_null(r);

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
        errx(-1, "filter error, can't exec regex: `%s'", errmsg);
        /* NOTREACHED */
    }

    return (ret);
}


static bool
t_cmp_eq(double d)
{
    return (d == 0);
}


static bool
t_cmp_ne(double d)
{
    return (d != 0);
}


static bool
t_cmp_lt(double d)
{
    return (d < 0);
}


static bool
t_cmp_le(double d)
{
    return (d <= 0);
}


static bool
t_cmp_gt(double d)
{
    return (d > 0);
}


static bool
t_cmp_ge(double d)
{
    return (d >= 0);
}

t_cmp_func *
t_invert(t_cmp_func *f)
{
    t_cmp_func *ret;

    assert_not_null(f);

    if (f == t_cmp_eq || f == t_cmp_ne)
        ret = f;
    else if (f == t_cmp_lt)
        ret = t_cmp_gt;
    else if (f == t_cmp_le)
        ret = t_cmp_ge;
    else if (f == t_cmp_gt)
        ret = t_cmp_lt;
    else if (f == t_cmp_ge)
        ret = t_cmp_le;
    else {
        (void)fprintf(stderr, "***internal filter error*** "
                "unexpected function in t_invert");
        assert_fail();
        /* NOTREACHED */
    }

    return (ret);
}


static bool
t_filter_eval_int_cmp(struct t_file *file,
        int i, const struct t_ast *rhs, t_cmp_func *f)
{
    const char *cs;
    bool ret = false;
    struct t_taglist *T = NULL;
    struct t_tag  *t;

    assert_not_null(file);
    assert_not_null(rhs);
    assert_not_null(f);

    switch (rhs->token->kind) {
    case T_FILENAME: /* FALLTHROUGH */
    case T_BACKEND:
        if (rhs->token->kind == T_FILENAME)
            cs = file->path;
        else
            cs = file->libid;
        ret = (*f)((double)(i - strtol(cs, NULL, 10)));
        break;
    case T_TAGKEY:
        T = file->get(file, rhs->token->val.str);
        if (T == NULL) {
            warnx("t_filter: `%s': %s",
                    file->path, t_error_msg(file));
            break;
        }
        if (T->count == 0)
            break;
        if (rhs->token->tidx == T_TOKEN_STAR) {
            char *s;
            switch (rhs->token->tidx_mod) {
            case T_TOKEN_STAR_NO_MOD:
                s = t_taglist_join(T, " - ");
                ret = (*f)((double)(i - strtol(s, NULL, 10)));
                freex(s);
                break;
            case T_TOKEN_STAR_OR_MOD:
                ret = false;
                TAILQ_FOREACH(t, T->tags, entries) {
                    if ((*f)((double)(i - strtol(t->val, NULL, 10)))) {
                        ret = true;
                        break;
                    }
                }
                break;
            case T_TOKEN_STAR_AND_MOD:
                ret = true;
                TAILQ_FOREACH(t, T->tags, entries) {
                    if (!(*f)((double)(i - strtol(t->val, NULL, 10)))) {
                        ret = false;
                        break;
                    }
                }
                break;
            }
        }
        else {
            t = t_taglist_tag_at(T, rhs->token->tidx);
            if (t != NULL)
                ret = (*f)((double)(i - strtol(t->val, NULL, 10)));
        }
        break;
    default:
        (void)fprintf(stderr, "***internal filter error*** "
                "unexpected token kind: rhs(%s)\n", rhs->token->str);
        assert_fail();
        /* NOTREACHED */
    }

    t_taglist_delete(T);
    return (ret);
}


static bool
t_filter_eval_double_cmp(struct t_file *file,
        double d, const struct t_ast *rhs, t_cmp_func *f)
{
    const char *cs;
    bool ret = false;
    struct t_taglist *T = NULL;
    struct t_tag  *t;

    assert_not_null(file);
    assert_not_null(rhs);
    assert_not_null(f);

    switch (rhs->token->kind) {
    case T_FILENAME: /* FALLTHROUGH */
    case T_BACKEND:
        if (rhs->token->kind == T_FILENAME)
            cs = file->path;
        else
            cs = file->libid;
        ret = (*f)(d - strtod(cs, NULL));
        break;
    case T_TAGKEY:
        T = file->get(file, rhs->token->val.str);
        if (T == NULL) {
            warnx("t_filter: `%s': %s",
                    file->path, t_error_msg(file));
            break;
        }
        if (T->count == 0)
            break;
        if (rhs->token->tidx == T_TOKEN_STAR) {
            char *s;
            switch (rhs->token->tidx_mod) {
            case T_TOKEN_STAR_NO_MOD:
                s = t_taglist_join(T, " - ");
                ret = (*f)(d - strtod(s, NULL));
                freex(s);
                break;
            case T_TOKEN_STAR_OR_MOD:
                ret = false;
                TAILQ_FOREACH(t, T->tags, entries) {
                    if ((*f)(d - strtod(t->val, NULL)))
                        ret = true;
                        break;
                    }
                break;
            case T_TOKEN_STAR_AND_MOD:
                ret = true;
                TAILQ_FOREACH(t, T->tags, entries) {
                    if (!(*f)(d - strtod(t->val, NULL))) {
                        ret = false;
                        break;
                    }
                }
                break;
            }
        }
        else {
            t = t_taglist_tag_at(T, rhs->token->tidx);
            if (t != NULL)
                ret = (*f)(d - strtod(t->val, NULL));
        }
        break;
    default:
        (void)fprintf(stderr, "***internal filter error*** "
                "unexpected token kind: rhs(%s)\n", rhs->token->str);
        assert_fail();
        /* NOTREACHED */
    }

    t_taglist_delete(T);
    return (ret);
}


static bool
t_filter_eval_str_cmp(struct t_file *file,
        const char *str, const struct t_ast *rhs,
        t_cmp_func *f)
{
    const char *cs;
    bool ret = false;
    struct t_taglist *T = NULL;
    struct t_tag  *t;

    assert_not_null(file);
    assert_not_null(rhs);
    assert_not_null(f);

    switch (rhs->token->kind) {
    case T_FILENAME: /* FALLTHROUGH */
    case T_BACKEND:
        if (rhs->token->kind == T_FILENAME)
            cs = file->path;
        else
            cs = file->libid;
        ret = (*f)((double)strcmp(str, cs));
        break;
    case T_TAGKEY:
        T = file->get(file, rhs->token->val.str);
        if (T == NULL) {
            warnx("t_filter: `%s': %s",
                    file->path, t_error_msg(file));
            break;
        }
        if (T->count == 0)
            break;
        if (rhs->token->tidx == T_TOKEN_STAR) {
            char *s;
            switch (rhs->token->tidx_mod) {
            case T_TOKEN_STAR_NO_MOD:
                s = t_taglist_join(T, " - ");
                ret = (*f)((double)strcmp(str, s));
                freex(s);
                break;
            case T_TOKEN_STAR_OR_MOD:
                TAILQ_FOREACH(t, T->tags, entries) {
                    if ((*f)((double)strcmp(str, t->val)))
                        ret = true;
                        break;
                    }
                break;
            case T_TOKEN_STAR_AND_MOD:
                ret = true;
                TAILQ_FOREACH(t, T->tags, entries) {
                    if (!(*f)((double)strcmp(str, t->val)))
                        ret = false;
                        break;
                    }
                break;
            }
        }
        else {
            t = t_taglist_tag_at(T, rhs->token->tidx);
            if (t != NULL)
                ret = (*f)((double)strcmp(str, t->val));
        }
        break;
    default:
        (void)fprintf(stderr, "***internal filter error*** "
                "unexpected token kind: rhs(%s)\n", rhs->token->str);
        assert_fail();
        /* NOTREACHED */
    }

    t_taglist_delete(T);
    return (ret);
}


static bool
t_filter_eval_undef_cmp(struct t_file *file,
        const struct t_ast *rhs, t_cmp_func *f)
{
    double def = 0;
    struct t_taglist *T = NULL;

    assert_not_null(file);
    assert_not_null(rhs);
    assert_not_null(f);

    switch (rhs->token->kind) {
    case T_TAGKEY:
        T = file->get(file, rhs->token->val.str);
        if (T == NULL) {
            warnx("t_filter: `%s': %s",
                    file->path, t_error_msg(file));
            return (false);
        }
        if (T->count == 0)
            def = 0;
        else
            def = 1;
        break;
    default:
        (void)fprintf(stderr, "***internal filter error*** "
                "unexpected token kind: rhs(%s)\n", rhs->token->str);
        assert_fail();
        /* NOTREACHED */
    }

    t_taglist_delete(T);
    return ((*f)(def));
}


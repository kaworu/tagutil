/*
 * t_renamer.c
 *
 * renamer for tagutil.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_strbuffer.h"
#include "t_lexer.h"
#include "t_renamer.h"


extern bool dflag;

/*
 * TODO
 */
_t__nonnull(1)
struct t_token * t_rename_lex_next_token(struct t_lexer *restrict L);

/* taken from mkdir(3) */
_t__nonnull(1)
static int build(char *path, mode_t omode);


bool
t_rename_safe(const char *restrict oldpath,
        const char *restrict newpath, struct t_error *restrict e)
{
    bool failed = false;
    struct stat st;
    char *olddirn, *newdirn;

    assert_not_null(oldpath);
    assert_not_null(newpath);
    t_error_clear(e);

    olddirn = t_dirname(oldpath);
    if (olddirn == NULL) {
        t_error_set(e, "%s", oldpath);
        return (false);
    }
    newdirn = t_dirname(newpath);
    if (newdirn == NULL) {
        t_error_set(e, "%s", newpath);
        freex(olddirn);
        return (false);
    }

    if (strcmp(olddirn, newdirn) != 0) {
    /* srcdir != destdir, we need to check if destdir is OK */
        if (dflag) {
        /* we are asked to actually create the directory */
            if (build(newdirn, S_IRWXU | S_IRWXG | S_IRWXO) == 0)
                failed = true;
        }
        if (stat(newdirn, &st) != 0) {
            if (errno == ENOENT)
                warnx("t_rename_safe: forgot -d?");
            failed = true;
        }
        else if (!S_ISDIR(st.st_mode)) {
            errno = ENOTDIR;
            failed = true;
        }
    }
    if (failed)
        t_error_set(e, "%s", newdirn);
    freex(olddirn);
    freex(newdirn);
    if (failed)
        return (false);

    if (stat(newpath, &st) == 0) {
        errno = EEXIST;
        t_error_set(e, "%s", newpath);
        return (false);
    }

    if (rename(oldpath, newpath) == -1) {
        t_error_set(e, "rename");
        return (false);
    }

    return (true);
}


struct t_token **
t_rename_parse(const char *restrict pattern)
{
    bool done;
    struct t_lexer *L;
    struct t_token **ret;
    size_t count, len;

    assert_not_null(pattern);

    L = t_lexer_new(pattern);
    (void)t_rename_lex_next_token(L);
    assert(L->current->kind == T_START);
    freex(L->current);

    count = 0;
    len   = 16;
    ret   = xcalloc(len + 1, sizeof(struct t_token *));

    done = false;
    while (!done) {
        if (t_rename_lex_next_token(L)->kind == T_END) {
            freex(L->current);
            done = true;
        }
        else {
            assert(L->current->kind == T_TAGKEY || L->current->kind == T_STRING);
            if (count == (len - 1)) {
                len = len * 2;
                ret = xrealloc(ret, (len + 1) * sizeof(struct t_token *));
            }
            ret[count++] = L->current;
        }
    }
    freex(L);

    ret[count] = NULL;
    return (ret);
}


struct t_token *
t_rename_lex_next_token(struct t_lexer *restrict L)
{
    int skip, i;
    bool done;
    struct t_token *t;
    assert_not_null(L);
    t = xcalloc(1, sizeof(struct t_token));

    /* check for T_START */
    if (L->cindex == -1) {
        (void)t_lexc(L);
        t->kind  = T_START;
		t->str   = "START";
        L->current = t;
        return (L->current);
    }

    skip = 0;
    t->start = L->cindex;
    switch (L->c) {
    case '\0':
        t->kind = T_END;
        t->str  = "END";
        t->end  = L->cindex;
        break;
    case '%':
        t_lex_tagkey(L, &t);
        break;
    default:
		t->kind = T_STRING;
		t->str  = "STRING";
        done = false;
        while (!done) {
            switch (L->c) {
            case '%': /* FALLTHROUGH */
            case '\0':
                done = true;
                break;
            case '\\':
                if (t_lexc(L) == '%') {
                    skip++;
                    (void)t_lexc(L);
                }
                break;
            default:
                (void)t_lexc(L);
            }
        }
        t->end = L->cindex - 1;
        assert(t->end >= t->start);
        t->slen = t->end - t->start + 1 - skip;
        t = xrealloc(t, sizeof(struct t_token) + t->slen + 1);
        t->value.str = (char *)(t + 1);
        t_lexc_move_to(L, t->start);
        i = 0;
        while (L->cindex <= t->end) {
            if (L->c == '\\') {
                if (t_lexc(L) != '%') {
                    /* rewind */
                    t_lexc_move(L, -1);
                    assert(L->c == '\\');
                }
            }
            t->value.str[i++] = L->c;
            (void)t_lexc(L);
        }
        t->value.str[i] = '\0';
        assert(strlen(t->value.str) == t->slen);
    }

    L->current = t;
    return (L->current);
}


char *
t_rename_eval(struct t_file *restrict file, struct t_token **restrict ts)
{
    const struct t_token *tkn;
    struct t_strbuffer *sb, *sbv;
    struct t_taglist *T;
    struct t_tag  *t;
    struct t_tagv *v;
    char *ret, *s;
    size_t len;
    int i;

    assert_not_null(ts);
    assert_not_null(file);
    t_error_clear(file);

    sb = t_strbuffer_new();
    tkn = *ts;
    while (tkn) {
        s = NULL;
        if (tkn->kind == T_TAGKEY) {
            T = file->get(file, tkn->value.str);
            if (T == NULL) {
                t_strbuffer_destroy(sb);
                return (NULL);
            }
            else if (T->tcount > 0) {
            /* tag exist */
                assert(T->tcount == 1);
                t = TAILQ_FIRST(T->tags);
                assert_not_null(t);
                assert(t->vcount > 0);
                if (tkn->tindex == -1 && t->vcount > 1) {
                /* user ask for *all* tag values */
                    sbv = t_strbuffer_new();
                    TAILQ_FOREACH(v, t->values, next) {
                        t_strbuffer_add(sbv, xstrdup(v->value), v->vlen);
                        if (v != TAILQ_LAST(t->values, t_tagv_q))
                            t_strbuffer_add(sbv, xstrdup(" - "), 3);
                    }
                    s = t_strbuffer_get(sbv);
                    len = sbv->len;
                    t_strbuffer_destroy(sbv);
                }
                else {
                /*
                 * requested one tag, or all but there is only one avaiable.
                 */
                    i = tkn->tindex == -1 ? 0 : tkn->tindex;
                    v = t_tag_value_by_idx(t, i);
                    if (v) {
                        s = xstrdup(v->value);
                        len = v->vlen;
                    }
                }
            }
            t_taglist_destroy(T);
        }
        if (s == NULL) {
            s = xstrdup(tkn->value.str);
            len = tkn->slen;
        }
        t_strbuffer_add(sb, s, len);
        /* go to next token */
        ts += 1;
        tkn = *ts;
    }

    ret = NULL;
    if (sb->len > MAXPATHLEN)
        t_error_set(file, "t_rename_eval result is too long (>MAXPATHLEN)");
    else
        ret = t_strbuffer_get(sb);

    t_strbuffer_destroy(sb);
    return (ret);
}


/*-
 * Copyright (c) 1983, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 */
#if 0
__FBSDID("$FreeBSD: src/bin/mkdir/mkdir.c,v 1.33 2006/10/10 20:18:20 ru Exp $");
#endif

/*
 * Returns 1 if a directory has been created,
 * 2 if it already existed, and 0 on failure.
 */
static int
build(char *path, mode_t omode)
{
	struct stat sb;
	mode_t numask, oumask;
	int first, last, retval;
	char *p;

	p = path;
	oumask = 0;
	retval = 1;
	if (p[0] == '/')		/* Skip leading '/'. */
		++p;
	for (first = 1, last = 0; !last ; ++p) {
		if (p[0] == '\0')
			last = 1;
		else if (p[0] != '/')
			continue;
		*p = '\0';
		if (!last && p[1] == '\0')
			last = 1;
		if (first) {
			/*
			 * POSIX 1003.2:
			 * For each dir operand that does not name an existing
			 * directory, effects equivalent to those caused by the
			 * following command shall occcur:
			 *
			 * mkdir -p -m $(umask -S),u+wx $(dirname dir) &&
			 *    mkdir [-m mode] dir
			 *
			 * We change the user's umask and then restore it,
			 * instead of doing chmod's.
			 */
			oumask = umask(0);
			numask = oumask & ~(S_IWUSR | S_IXUSR);
			(void)umask(numask);
			first = 0;
		}
		if (last)
			(void)umask(oumask);
		if (mkdir(path, last ? omode : S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
			if (errno == EEXIST || errno == EISDIR) {
				if (stat(path, &sb) < 0) {
					warn("rename_build: %s", path);
					retval = 0;
					break;
				} else if (!S_ISDIR(sb.st_mode)) {
					if (last)
						errno = EEXIST;
					else
						errno = ENOTDIR;
					retval = 0;
					break;
				}
				if (last)
					retval = 2;
			} else {
				retval = 0;
				break;
			}
		}
		if (!last)
		    *p = '/';
	}
	if (!first && !last)
		(void)umask(oumask);
	return (retval);
}


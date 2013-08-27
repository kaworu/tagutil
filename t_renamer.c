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



/*
 * TODO
 */
_t__nonnull(1)
struct t_token * t_rename_lex_next_token(struct t_lexer *L);

/* taken from mkdir(3) */
_t__nonnull(1)
static int build(char *path, mode_t omode);


bool
t_rename_safe(struct t_file *file, const char *newpath)
{
	extern bool dflag;
	bool failed = false;
	struct stat st;
	const char *odir, *ndir;

	assert_not_null(file);
	assert_not_null(file->path);
	assert_not_null(newpath);

	odir = t_dirname(file->path);
	if (odir == NULL) {
		t_error_set(file, "%s", file->path);
		return (false);
	}
	ndir = t_dirname(newpath);
	if (ndir == NULL) {
		t_error_set(file, "%s", newpath);
		return (false);
	}

	if (strcmp(odir, ndir) != 0) {
		/* srcdir != destdir, we need to check if destdir is OK */
		if (dflag) { /* we are asked to create the directory */
			char	*d;

			d = strdup(ndir);
			if (d == NULL)
				err(ENOMEM, "strdup");
			(void)build(d, S_IRWXU | S_IRWXG | S_IRWXO);
			free(d);
		}
		if (stat(ndir, &st) != 0) {
			if (errno == ENOENT)
				t_error_set(file, "forgot -d?");
			failed = true;
		} else if (!S_ISDIR(st.st_mode)) {
			errno  = ENOTDIR;
			failed = true;
		}
	}
	if (failed) {
		t_error_set(file, "%s", ndir);
		return (false);
	}

	if (stat(newpath, &st) == 0) {
		errno = EEXIST;
		t_error_set(file, "%s", newpath);
		return (false);
	}

	if (rename(file->path, newpath) == -1) {
		t_error_set(file, "rename");
		return (false);
	}

	free(file->path);
	file->path = xstrdup(newpath);
	return (true);
}


struct t_token **
t_rename_parse(const char *pattern)
{
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

    while (t_rename_lex_next_token(L)->kind != T_END) {
            assert(L->current->kind == T_TAGKEY || L->current->kind == T_STRING);
            if (count == (len - 1)) {
                len = len * 2;
                ret = xrealloc(ret, (len + 1) * sizeof(struct t_token *));
            }
            ret[count++] = L->current;
    }
    freex(L->current);
    t_lexer_destroy(L);

    ret[count] = NULL;
    return (ret);
}


struct t_token *
t_rename_lex_next_token(struct t_lexer *L)
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
        t_lex_tagkey(L, &t, !T_LEXER_ALLOW_STAR_MOD);
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
	/* FIXME: use sbuf(9), t_strbuff, avoid two loops, realloc etc. */
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
t_rename_eval(struct t_file *file, struct t_token **ts)
{
    const struct t_token *tkn;
    struct t_strbuffer *sb;
    struct t_taglist *T;
    struct t_tag *t;
    char *ret, *s, *slash;
    size_t len;

    assert_not_null(ts);
    assert_not_null(file);
    t_error_clear(file);

    sb = t_strbuffer_new();
    tkn = *ts;
    while (tkn != NULL) {
        s = NULL;
        if (tkn->kind == T_TAGKEY) {
            T = file->get(file, tkn->value.str);
            if (T == NULL) {
                t_strbuffer_destroy(sb);
                return (NULL);
            }
            else if (T->count > 0) {
            /* tag exist */
                if (tkn->tidx == T_TOKEN_STAR) {
                /* user ask for *all* tag values */
                    s = t_taglist_join(T, " - ");
                    len = strlen(s);
                }
                else {
                /* requested one tag */
                    t = t_taglist_tag_at(T, tkn->tidx);
                    if (t != NULL) {
                        s = xstrdup(t->value);
                        len = t->valuelen;
                    }
                }
            }
            t_taglist_destroy(T);
            if (s != NULL) {
            /* check for slash in tag value */
                slash = strchr(s, '/');
                if (slash) {
                    warnx("rename_eval: `%s': tag `%s' has / in value, "
                            "replacing by `-'", file->path, tkn->value.str);
                    do {
                        *slash = '-';
                        slash = strchr(slash, '/');
                    } while (slash);
                }
            }
        }
        if (s != NULL)
            t_strbuffer_add(sb, s, len, T_STRBUFFER_FREE);
        else
            t_strbuffer_add(sb, tkn->value.str, tkn->slen, T_STRBUFFER_NOFREE);
        /* go to next token */
        ts += 1;
        tkn = *ts;
    }

    ret = NULL;
    if (sb->len > MAXPATHLEN)
        t_error_set(file, "t_rename_eval result is too long (>MAXPATHLEN)");
    else
        ret = t_strbuffer_get(sb);
    return (ret);
}


/*-
 * Copyright (c) 1983, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
					warn("build: %s", path);
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


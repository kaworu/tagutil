/*
 * t_renamer.c
 *
 * renamer for tagutil.
 */
#include "t_config.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "t_toolkit.h"
#include "t_lexer.h"
#include "t_renamer.h"


extern bool dflag;

/*
 * TODO
 */
_t__nonnull(1)
struct token * rename_lex_next_token(struct lexer *restrict L);

/* taken from mkdir(3) */
_t__nonnull(1)
static inline int build(char *path, mode_t omode);


void
rename_safe(const char *restrict oldpath,
        const char *restrict newpath)
{
    bool failed = false;
    struct stat st;
    char *olddirn, *newdirn;

    assert_not_null(oldpath);
    assert_not_null(newpath);

    olddirn = xdirname(oldpath);
    newdirn = xdirname(newpath);
    if (strcmp(olddirn, newdirn) != 0) {
    /* srcdir != destdir, we need to check if destdir is OK */
        if (dflag) {
        /* we are asked to actually create the directory */
            if (build(newdirn, S_IRWXU | S_IRWXG | S_IRWXO) == 0) /* failure */
                failed = true;
        }
        if (stat(newdirn, &st) != 0) {
            if (errno == ENOENT)
                warnx("rename_safe: forgot -d?");
            failed = true;
        }
        else if (!S_ISDIR(st.st_mode)) {
            errno = ENOTDIR;
            failed = true;
        }
    }
    if (failed)
        err(errno, "%s", newdirn);
    xfree(olddirn);
    xfree(newdirn);

    if (stat(newpath, &st) == 0)
        err(errno = EEXIST, "%s", newpath);

    if (rename(oldpath, newpath) == -1)
        err(errno, "rename");
}


struct token *
rename_lex_next_token(struct lexer *restrict L)
{
    int skip, i;
    bool done;
    struct token *t;
    assert_not_null(L);
    t = xcalloc(1, sizeof(struct token));

    /* check for TSTART */
    if (L->cindex == -1) {
        (void)lexc(L);
        t->kind  = TSTART;
		t->str   = "START";
        L->current = t;
        return (L->current);
    }

    skip = 0;
    t->start = L->cindex;
    switch (L->c) {
    case '\0':
        t->kind = TEND;
        t->str  = "END";
        t->end  = L->cindex;
        break;
    case '%':
        lex_tagkey(L, &t);
        break;
    default:
		t->kind = TSTRING;
		t->str  = "STRING";
        done = false;
        while (!done) {
            switch (L->c) {
            case '%': /* FALLTHROUGH */
            case '\0':
                done = true;
                break;
            case '\\':
                if (lexc(L) == '%') {
                    skip++;
                    (void)lexc(L);
                }
                break;
            default:
                (void)lexc(L);
            }
        }
        t->end = L->cindex - 1;
        assert(t->end >= t->start);
        t->alloclen = t->end - t->start + 2 - skip;
        t = xrealloc(t, sizeof(struct token) + t->alloclen);
        t->value.str = (char *)(t + 1);
        L->cindex = t->start;
        L->c = L->source[L->cindex];
        i = 0;
        while (L->cindex <= t->end) {
            if (L->c == '\\') {
                if (lexc(L) != '%') {
                    /* rewind */
                    L->cindex -= 2;
                    L->c = '\\';
                }
            }
            t->value.str[i++] = L->c;
            (void)lexc(L);
        }
        t->value.str[i] = '\0';
        assert(strlen(t->value.str) + 1 == t->alloclen);
    }

    L->current = t;
    return (L->current);
}


char *
rename_eval(struct tfile *restrict file, const char *restrict pattern)
{
    struct lexer *L;
    const size_t len = MAXPATHLEN;
    char *ret, *s;
    bool done;

    assert_not_null(file);
    assert_not_null(pattern);

    L = new_lexer(pattern);
    ret = xcalloc(len, sizeof(char));

    (void)rename_lex_next_token(L);
    assert(L->current->kind == TSTART);
    xfree(L->current);
    done = false;
    while (!done) {
        if (rename_lex_next_token(L)->kind == TEND)
            done = true;
        else {
            s = NULL;
            if (L->current->kind == TTAGKEY)
                s = file->get(file, L->current->value.str);

            if (s == NULL)
                s = L->current->value.str;
            if (strlcat(ret, s, len) >= len) {
                warnx("rename_eval: pattern too long (>MAXPATHLEN) for `%s'",
                        file->path);
                xfree(ret);
                done = true;
            }
            if (s != L->current->value.str)
                xfree(s);
        }
        xfree(L->current);
    }
    xfree(L);

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
static inline int
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


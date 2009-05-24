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
#include "t_renamer.h"


extern bool dflag, Dflag;

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


char *
rename_eval(struct tfile *restrict file, const char *restrict pattern)
{
    const size_t len = MAXPATHLEN;
    char *ret;
    char *p, *k, *palloc;
    char *start, *end;
    char *key, *value;
    size_t keylen;
    bool running = true;
    enum {
        SEARCH_TAG, GET_KEY, GET_KEY_BRACE, SET_TAG, TEARDOWN,
        TOO_LONG, MISSING_BRACE
    } fsm;

    ret = xcalloc(len, sizeof(char));
    palloc = p = xstrdup(pattern);
    fsm = SEARCH_TAG;

    while (running) {
        switch (fsm) {
        case SEARCH_TAG:
            start = strchr(p, '%');
            if (start == NULL)
            /* no more % */
                fsm = TEARDOWN;
            else if (start != p && start[-1] == '\\') {
            /* \% escape */
                start[-1] = '%';
                start[ 0] = '\0';
                if (strlcat(ret, p, len) >= len)
                    fsm = TOO_LONG;
                else
                    p = start + 1;
            }
            else {
            /* %tag or %{tag} */
                start[0] = '\0';
                if (strlcat(ret, p, len) >= len)
                    fsm = TOO_LONG;
                else {
                    if (start[1] == '{') {
                        p = start + 2;
                        fsm = GET_KEY_BRACE;
                    }
                    else {
                        p = start + 1;
                        fsm = GET_KEY;
                    }
                }
            }
            break;
        case GET_KEY:
            for (end = p; isalnum(*end) || *end == '_' || *end == '-'; end++)
                continue;
            if (end == p)
                warnx("rename_eval: %% without tag name (%d)", (int)(p - palloc));
            keylen = (size_t)(end - p) + 1;
            key = xcalloc(keylen, sizeof(char));
            (void)strlcpy(key, p, keylen);
            p = end;
            fsm = SET_TAG;
            break;
        case GET_KEY_BRACE:
            keylen = strlen(p) + 1;
            key = xcalloc(keylen, sizeof(char));
            k = key;
            for (end = p; *end != '\0' && *end != '}'; end++) {
                if (*end == '\\' && end[1] == '}') {
                    *k = '}';
                    k++;
                    end++;
                }
                else {
                    *k = *end;
                    k++;
                }
            }
            if (*end == '\0') {
                xfree(key);
                fsm = MISSING_BRACE;
            }
            else {
                fsm = SET_TAG;
                p = end + 1;
            }
            break;
        case SET_TAG:
            value = file->get(file, key);
            if (value == NULL) {
                warnx("rename_eval: no tag `%s' for `%s', using tag key instead.",
                        key, file->path);
                value = key;
            }
            else {
                if (dflag && !Dflag && strchr(value, '/'))
                    errx(-1, "rename_eval: `%s': tag `%s' has / in value, fix it or"
                            " use -D", file->path, key);
                xfree(key);
            }
            if (strlcat(ret, value, len) >= len)
                fsm = TOO_LONG;
            else
                fsm = SEARCH_TAG;
            xfree(value);
            break;
        case TEARDOWN:
            if (strlcat(ret, p, len) >= len)
                fsm = TOO_LONG;
            else
                running = false;
            break;
        case TOO_LONG:
            warnx("rename_eval: pattern too long (>MAXPATHLEN) for `%s'",
                    file->path);
            running = false;
            xfree(ret);
            break;
        case MISSING_BRACE:
            errx(EINVAL, "rename_eval: missing }");
            /* NOTREACHED */
        }
    }
    xfree(palloc);

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


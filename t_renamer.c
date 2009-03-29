/*
 * t_renamer.c
 *
 * renamer for tagutil.
 */
#include "t_config.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "t_toolkit.h"
#include "t_renamer.h"


/* taken from mkdir(3) */
__t__nonnull(1)
static int build(char *path, mode_t omode);


void
safe_rename(bool dflag, const char *restrict oldpath,
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
        if (stat(newdirn, &st) != 0)
            failed = true;
        else if (!S_ISDIR(st.st_mode)) {
            errno = ENOTDIR;
            failed = true;
        }
    }
    if (failed)
        err(errno, "%s", newdirn);
    free(olddirn);
    free(newdirn);

    if (stat(newpath, &st) == 0)
        err(errno = EEXIST, "%s", newpath);

    if (rename(oldpath, newpath) == -1)
        err(errno, NULL);
}


char *
eval_tag(const char *restrict pattern, const TagLib_Tag *restrict tags)
#define CLEARBUF    do {                                \
                        if (bufc > 0) {                 \
                            buf[bufc++] = '\0';         \
                            concat(&ret, &retlen, buf); \
                            bufc = 0;                   \
                        }                               \
                    } while (/*CONSTCOND*/0)
{
    char *ret;
    char buf[BUFSIZ];
    size_t retlen, bufc, patternlen, i;

    assert_not_null(pattern);
    assert_not_null(tags);

    retlen = 1;
    patternlen = strlen(pattern);
    bufc = 0;

    ret = xcalloc(retlen, sizeof(char));

    for (i = 0; i < patternlen; i++) {
        if (bufc == len(buf) - 2)
            CLEARBUF;

        if (pattern[i] == '%') {
            switch (pattern[i + 1]) {
            case '%':
                buf[bufc++] = '%';
                break;
            case 'A':
                CLEARBUF;
                concat(&ret, &retlen, taglib_tag_artist(tags));
                break;
            case 'a':
                CLEARBUF;
                concat(&ret, &retlen, taglib_tag_album(tags));
                break;
            case 'c':
                CLEARBUF;
                concat(&ret, &retlen, taglib_tag_comment(tags));
                break;
            case 'g':
                CLEARBUF;
                concat(&ret, &retlen, taglib_tag_genre(tags));
                break;
            case 'T':
                CLEARBUF;
                (void)snprintf(buf, len(buf), "%02u", taglib_tag_track(tags));
                concat(&ret, &retlen, buf);
                break;
            case 't':
                CLEARBUF;
                concat(&ret, &retlen, taglib_tag_title(tags));
                break;
            case 'y':
                CLEARBUF;
                (void)snprintf(buf, len(buf), "%02u", taglib_tag_year(tags));
                concat(&ret, &retlen, buf);
                break;
            default:
                warnx("%c%c: unknown keyword at %zd, skipping.", pattern[i], pattern[i + 1], i);
            }
            i += 1; /* skip keyword */
        }
        else
            buf[bufc++] = pattern[i];
    }

    if (bufc > 0)
        CLEARBUF;

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
int
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
					warn("%s", path);
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

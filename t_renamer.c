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


void
safe_rename(const char *restrict oldpath, const char *restrict newpath)
{
    struct stat st;
    char *olddirn, *newdirn;

    assert_not_null(oldpath);
    assert_not_null(newpath);

    olddirn = xdirname(oldpath);
    newdirn = xdirname(newpath);
    if (strcmp(olddirn, newdirn) != 0) {
    /* srcdir != destdir, we need to check if destdir is OK */
        if (stat(newdirn, &st) == 0 && S_ISDIR(st.st_mode))
            /* destdir exists, so let's try */;
        else {
            err(errno, "can't rename \"%s\" to \"%s\" destination directory doesn't exist",
                    oldpath, newpath);
        }
    }
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

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


/*
 * replace each tagutil keywords by their value. see k* keywords.
 *
 * This function use the following algorithm:
 * 1) start from the end of pattern.
 * 2) search backward for a keyword that we use (like %a, %t etc.)
 * 3) when a keyword is found, replace it by it's value.
 * 4) go to step 2) until we process all the pattern.
 *
 * this algorithm ensure that we expend all and only the keywords that are
 * in the initial pattern (we don't expend keywords in values. for example in
 * an artist value like "foo %t" we won't expend the %t into title).
 * It also ensure that we can expend the same keyword more than once (for
 * example in a pattern like "%t %t %t"  the title tag is "MyTitle" we expend
 * the pattern into "MyTitle MyTitle MyTitle").
 */
char *
eval_tag(const char *restrict pattern, const TagLib_Tag *restrict tag)
{
    regmatch_t matcher;
    int cursor;
    char *c;
    char *result;
    char buf[5]; /* used to convert track/year tag into string. need to be fixed in year > 9999 */

    assert_not_null(pattern);
    assert_not_null(tag);

    /* init result with pattern */
    result = xstrdup(pattern);

#define _REPLACE_BY_STRING_IF_MATCH(pattern, x)                                     \
    do {                                                                            \
            if (strncmp(c, pattern, strlen(pattern)) == 0) {                        \
                matcher.rm_so = cursor;                                             \
                matcher.rm_eo = cursor + strlen(pattern);                           \
                inplacesub_match(&result, &matcher, (taglib_tag_ ## x)(tag));       \
                /* the following line *is* important. If you don't understand       \
                 * why, try tagutil -r '%%a' <file> with the album tag of           \
                 * file set to "An Album". eval_tag() will replace "%%a" by         \
                 * "%An Album" and then cursor will be on the % if we don't         \
                 * cursor--. Then eval_tag() will expend the %A and we don't        \
                 * want to. so cursor-- prevent this and ensure that the first      \
                 * expension will be safe and not modified.                         \
                 */                                                                 \
                cursor--;                                                           \
                goto next_loop_iter;                                                \
            }                                                                       \
    } while (/*CONSTCOND*/0)
#define _REPLACE_BY_INT_IF_MATCH(pattern, x)                                        \
    do {                                                                            \
            if (strncmp(c, pattern, strlen(pattern)) == 0) {                        \
                matcher.rm_so = cursor;                                             \
                matcher.rm_eo = cursor + strlen(pattern);                           \
                (void)snprintf(buf, len(buf), "%02u", (taglib_tag_ ## x)(tag));     \
                inplacesub_match(&result, &matcher, buf);                           \
                cursor--;                                                           \
                goto next_loop_iter;                                                \
            }                                                                       \
    } while (/*CONSTCOND*/0)

    for(cursor = strlen(pattern) - 2; cursor >= 0; cursor--) {
        c = &result[cursor];
        if (*c == '%') {
            /*TODO: implement kESCAPE %% */
            _REPLACE_BY_STRING_IF_MATCH (kTITLE,   title);
            _REPLACE_BY_STRING_IF_MATCH (kALBUM,   album);
            _REPLACE_BY_STRING_IF_MATCH (kARTIST,  artist);
            _REPLACE_BY_STRING_IF_MATCH (kCOMMENT, comment);
            _REPLACE_BY_STRING_IF_MATCH (kGENRE,   genre);
            _REPLACE_BY_INT_IF_MATCH    (kYEAR,    year);
            _REPLACE_BY_INT_IF_MATCH    (kTRACK,   track);
        }
next_loop_iter:
        /* NOOP */;
    }

    return (result);
}

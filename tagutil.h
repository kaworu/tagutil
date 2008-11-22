#ifndef T_TAGUTIL_H
#define T_TAGUTIL_H
/*
 * tagutil main header.
 */
#include "t_config.h"

#include <stdio.h>
#include <stdbool.h>

#include <tag_c.h>


/*
 * tagutil_f is the type of function that implement actions in tagutil.
 * 1rst arg: the current file's name
 * 2nd  arg: the TagLib_File of the current file
 * 3rd  arg: the arg supplied to the action (for example, if -y is given as
 * option then action is tagutil_year and arg is the new value for the
 * year tag.
 */
typedef bool (*tagutil_f)(const char *restrict, TagLib_File *restrict, const void *restrict);

/**********************************************************************/


/*
 * show usage and exit.
 */
__t__dead2
void usage(void);


/* FILE FUNCTIONS */

/*
 * create a temporary file in $TMPDIR. if $TMPDIR is not set, /tmp is
 * used. return the full path to the temp file created.
 *
 * returned value has to be freed.
 */
char * create_tmpfile(void);


/* TAG FUNCTIONS */

/*
 * call "$EDITOR path" (the environment variable $EDITOR must be set).
 */
__t__nonnull(1)
bool user_edit(const char *restrict path);


/* TAGUTIL FUNCTIONS */

/*
 * print the given file's tag to stdin.
 */
__t__nonnull(1) __t__nonnull(2)
bool tagutil_print(const char *restrict path, TagLib_File *restrict f,
        __t__unused const void *restrict arg);

/*
 * print the given file's tag and prompt to ask if tag edit is needed. if
 * answer is yes create a tempfile, fill is with infos, run $EDITOR, then
 * parse the tempfile and update the file's tag.
 */
__t__nonnull(1) __t__nonnull(2)
bool tagutil_edit(const char *restrict path, TagLib_File *restrict f,
        __t__unused const void *restrict arg);

/*
 * rename the file at path with the given pattern arg. the pattern can use
 * some keywords for tags (see usage()).
 */
__t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
bool tagutil_rename(const char *restrict path, TagLib_File *restrict f,
        const void *restrict arg);

/*
 * print given path if the file match the given ast (arg).
 */
__t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
bool tagutil_filter(const char *restrict path, TagLib_File *restrict f,
        const void *restrict arg);

__t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
bool tagutil_title(const char *restrict path, TagLib_File *restrict f,
        const void *restrict arg);

__t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
bool tagutil_album(const char *restrict path, TagLib_File *restrict f,
        const void *restrict arg);

__t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
bool tagutil_artist(const char *restrict path, TagLib_File *restrict f,
        const void *restrict arg);

__t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
bool tagutil_year(const char *restrict path, TagLib_File *restrict f,
        const void *restrict arg);

__t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
bool tagutil_track(const char *restrict path, TagLib_File *restrict f,
        const void *restrict arg);

__t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
bool tagutil_comment(const char *restrict path, TagLib_File *restrict f,
        const void *restrict arg);

__t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
bool tagutil_genre(const char *restrict path, TagLib_File *restrict f,
        const void *restrict arg);


#endif /* not T_TAGUTIL_H */

#ifndef T_TAGUTIL_H
#define T_TAGUTIL_H
/*
 * tagutil main header.
 */
#include "t_config.h"

#include <stdio.h>
#include <stdbool.h>

#include <tag_c.h> /* XXX */
#include "t_file.h"


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
bool tagutil_print(const char *restrict path, TagLib_File *restrict f);

/*
 * print the given file's tag and prompt to ask if tag edit is needed. if
 * answer is yes create a tempfile, fill is with infos, run $EDITOR, then
 * parse the tempfile and update the file's tag.
 */
__t__nonnull(1) __t__nonnull(2)
bool tagutil_edit(const char *restrict path, TagLib_File *restrict f);

/*
 * rename the file at path with the given pattern arg. the pattern can use
 * some keywords for tags (see usage()).
 */
__t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
bool tagutil_rename(const char *restrict path, TagLib_File *restrict f,
        const char *restrict pattern);

/*
 * print given path if the file match the given ast (arg).
 */
__t__nonnull(1) __t__nonnull(2)
bool tagutil_filter(struct tfile *file, const struct ast *restrict ast);

__t__nonnull(1) __t__nonnull(2)
bool tagutil_title(TagLib_File *restrict f, const char *restrict title);

__t__nonnull(1) __t__nonnull(2)
bool tagutil_album(TagLib_File *restrict f, const char *restrict album);

__t__nonnull(1) __t__nonnull(2)
bool tagutil_artist(TagLib_File *restrict f, const char *restrict artist);

__t__nonnull(1)
bool tagutil_year(TagLib_File *restrict f, int year);

__t__nonnull(1)
bool tagutil_track(TagLib_File *restrict f, int track);

__t__nonnull(1) __t__nonnull(2)
bool tagutil_comment(TagLib_File *restrict f, const char *restrict comment);

__t__nonnull(1) __t__nonnull(2)
bool tagutil_genre(TagLib_File *restrict f, const char *restrict genre);


#endif /* not T_TAGUTIL_H */

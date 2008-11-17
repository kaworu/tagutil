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
 * you should not modifiy them, eval_tag() implementation might be depend.
 */
#define kTITLE      "%t"
#define kALBUM      "%a"
#define kARTIST     "%A"
#define kYEAR       "%y"
#define kTRACK      "%T"
#define kCOMMENT    "%c"
#define kGENRE      "%g"


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
 * rename path to new_path. err(3) if new_path already exist.
 */
__t__nonnull(1) __t__nonnull(2)
void safe_rename(const char *restrict oldpath, const char *restrict newpath);

/*
 * create a temporary file in $TMPDIR. if $TMPDIR is not set, /tmp is
 * used. return the full path to the temp file created.
 *
 * returned value has to be freed.
 */
char * create_tmpfile(void);


/* TAG FUNCTIONS */

/*
 * return a char* that contains all tag infos.
 *
 * returned value has to be freed.
 */
__t__nonnull(1)
char * printable_tag(const TagLib_Tag *restrict tag);

/*
 * call "$EDITOR path" (the environment variable $EDITOR must be set).
 */
__t__nonnull(1)
bool user_edit(const char *restrict path);

/*
 * read stream and tag. the format of the text should bethe same as
 * tagutil_print.
 * return false if a line can't be parsed, true if all went smoothly.
 */
__t__nonnull(1) __t__nonnull(2)
bool update_tag(TagLib_Tag *restrict tag, FILE *restrict stream);

/*
 * replace each tagutil keywords by their value. see k* keywords.
 */
__t__nonnull(1) __t__nonnull(2)
char * eval_tag(const char *restrict pattern, const TagLib_Tag *restrict tag);


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

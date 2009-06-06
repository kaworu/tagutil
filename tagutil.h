#ifndef T_TAGUTIL_H
#define T_TAGUTIL_H
/*
 * tagutil main header.
 */
#include "t_config.h"

#include <stdio.h>
#include <stdbool.h>

#include "t_file.h"


/*
 * show usage and exit.
 */
_t__dead2
void usage(void);


/* FILE FUNCTIONS */

/*
 * create a temporary file in $TMPDIR. if $TMPDIR is not set, /tmp is
 * used. return the full path to the temp file created.
 *
 * returned value has to be free()d.
 */
char * create_tmpfile(void);


/* TAG FUNCTIONS */

/*
 * call "$EDITOR path" (the environment variable $EDITOR must be set).
 */
_t__nonnull(1)
bool user_edit(const char *restrict path);


/* TAGUTIL FUNCTIONS */

/*
 * print the given file's tag to stdin.
 */
_t__nonnull(1)
bool tagutil_print(const struct tfile *restrict file);

/*
 * parse the given file (path) and update the file's tag.
 */
_t__nonnull(1) _t__nonnull(2)
bool tagutil_load(struct tfile *restrict file, const char *restrict path);

/*
 * print the given file's tag and prompt to ask if tag edit is needed. if
 * answer is yes create a tempfile, fill is with infos, run $EDITOR, then
 * parse the tempfile and update the file's tag.
 */
_t__nonnull(1)
bool tagutil_edit(struct tfile *restrict file);

/*
 * rename the file at path with the given pattern arg. the pattern can use
 * some keywords for tags (see usage()).
 */
_t__nonnull(1) _t__nonnull(2)
bool tagutil_rename(struct tfile *restrict file, const char *restrict pattern);

/*
 * print given path if the file match the given ast (arg).
 */
_t__nonnull(1) _t__nonnull(2)
bool tagutil_filter(const struct tfile *restrict file,
        const struct ast *restrict ast);

#endif /* not T_TAGUTIL_H */

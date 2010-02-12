#ifndef T_TAGUTIL_H
#define T_TAGUTIL_H
/*
 * tagutil main header.
 */
#include <stdbool.h>

#include "t_config.h"
#include "t_file.h"
#include "t_lexer.h"
#include "t_parser.h"


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
bool tagutil_print(struct t_file *restrict file);

/*
 * parse the given file (path) and update the file's tag.
 */
_t__nonnull(1) _t__nonnull(2)
bool tagutil_load(struct t_file *restrict file, const char *restrict path);

/*
 * print the given file's tag and prompt to ask if tag edit is needed. if
 * answer is yes create a tempfile, fill is with infos, run $EDITOR, then
 * parse the tempfile and update the file's tag.
 */
_t__nonnull(1)
bool tagutil_edit(struct t_file *restrict file);

/*
 * rename the file.
 *
 * the pattern tknary is the result of the pattern string compiled by
 * t_rename_parse().
 */
_t__nonnull(1) _t__nonnull(2)
bool tagutil_rename(struct t_file *restrict file, struct t_token **restrict tknary);

/*
 * print given path if the file match the given ast (arg).
 */
_t__nonnull(1) _t__nonnull(2)
bool tagutil_filter(struct t_file *restrict file,
        const struct t_ast *restrict ast);

#endif /* not T_TAGUTIL_H */

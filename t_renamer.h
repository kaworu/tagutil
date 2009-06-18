#ifndef T_RENAMER_H
#define T_RENAMER_H
/*
 * t_renamer.h
 *
 * renamer for tagutil.
 */
#include "t_config.h"
#include "t_error.h"
#include "t_file.h"
#include "t_lexer.h"


/*
 * create a token array usable for t_rename_eval() from given pattern.
 * the array is NULL terminated.
 *
 * The tag keys in pattern must look like shell variables (i.e. %artist or/and
 * %{album}). If the tag key is not defined for a file, the tag key is replaced
 * by its name (i.e. "%{undefined}" or "%undefined" becomes "undefined"). If
 * you want a litteral %, use \%
 *
 * return value and all its elements has to be free()d.
 */
_t__nonnull(1)
struct t_token ** t_rename_parse(const char *restrict pattern);

/*
 * eval the given token array in the context of given t_file.
 *
 * On error t_error_msg(file) is set and NULL is returned. Otherwhise the
 * result is returned.
 *
 * returned value has to be free()d.
 */
_t__nonnull(1) _t__nonnull(2)
char * t_rename_eval(struct t_file *restrict file, struct t_token **restrict ts);

/*
 * rename path to new_path.
 *
 * return false on error and set t_error_msg(e) if not NULL and errno.
 * return true otherwise.
 */
_t__nonnull(1) _t__nonnull(2)
bool t_rename_safe(const char *restrict oldpath, const char *restrict newpath,
        struct t_error *restrict e);

#endif /* not T_RENAMER_H */

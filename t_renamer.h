#ifndef T_RENAMER_H
#define T_RENAMER_H
/*
 * t_renamer.h
 *
 * renamer for tagutil.
 */
#include "t_config.h"
#include "t_tune.h"
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
struct t_token	**t_rename_parse(const char *pattern);

/*
 * eval the given token array in the context of given t_tune.
 *
 * On error NULL is returned. Otherwhise the result is returned.
 *
 * returned value has to be free()d.
 */
char	*t_rename_eval(struct t_tune *tune, struct t_token **ts);

/*
 * rename path to new_path.
 *
 * return 0 on error and set and errno, return 1 otherwise.
 */
int	t_rename_safe(const char *oldpath, const char *newpath);

#endif /* not T_RENAMER_H */

#ifndef T_RENAMER_H
#define T_RENAMER_H
/*
 * t_renamer.h
 *
 * renamer for tagutil.
 */
#include "t_config.h"
#include "t_tune.h"


/* declaration of a rename pattern type */
struct t_rename_pattern;


/*
 * create a pattern usable for t_rename().
 *
 * The tag keys in pattern should look like shell variables (%artist or/and
 * %{album}). If the tag key is not defined for a file, the tag key is replaced
 * by its name (i.e. "%{undefined}" or "%undefined" becomes "undefined"). If
 * you want a litteral %, use \%
 *
 * return value and all its elements has to be given to
 * t_rename_pattern_delete() after use.
 */
struct t_rename_pattern	*t_rename_parse(const char *pattern);

/*
 * rename the given file (tune) using pattern.
 *
 * This routine will require user confirmation unless the Yflag or Nflag was
 * set.
 *
 * @param tune
 *   The file to rename.
 *
 * @param pattern
 *   The pattern for the new filename.
 *
 * @return
 *   -1 on error, 0 on success.
 */
int	t_rename(struct t_tune *tune, const struct t_rename_pattern *pattern);

/*
 * free all memory associated with a pattern.
 */
void	t_rename_pattern_delete(struct t_rename_pattern *pattern);

#endif /* not T_RENAMER_H */

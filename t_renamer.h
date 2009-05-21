#ifndef T_RENAMER_H
#define T_RENAMER_H
/*
 * t_renamer.c
 *
 * renamer for tagutil.
 */
#include "t_config.h"

#include "t_file.h"


/*
 * rename path to new_path. err(3) if new_path already exist.
 */
_t__nonnull(1) _t__nonnull(2)
void rename_safe(const char *restrict oldpath, const char *restrict newpath);


/*
 * replace each tagutil keywords by their value. see k* keywords.
 */
_t__nonnull(1) _t__nonnull(2)
char * rename_eval(struct tfile *restrict file, const char *restrict pattern);


#endif /* not T_RENAMER_H */

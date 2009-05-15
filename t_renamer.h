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
 * rename path to new_path. err(3) if new_path already exist.
 */
__t__nonnull(1) __t__nonnull(2)
void safe_rename(const char *restrict oldpath, const char *restrict newpath);


/*
 * replace each tagutil keywords by their value. see k* keywords.
 */
__t__nonnull(1) __t__nonnull(2)
char * eval_tag(struct tfile *restrict file, const char *restrict pattern);


#endif /* not T_RENAMER_H */

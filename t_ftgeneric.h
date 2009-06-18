#ifndef T_FTGENERIC_H
#define T_FTGENERIC_H
/*
 * t_ftgeneric.h
 *
 * a generic tagutil backend, using TagLib.
 */
#include "t_config.h"
#include "t_file.h"


#if defined(WITH_TAGLIB)

void ftgeneric_init(void);

_t__nonnull(1)
struct tfile * ftgeneric_new(const char *restrict path);

#else /* not WITH_TAGLIB */

# define ftgeneric_init()
# define ftgeneric_new(path) NULL

#endif /* WITH_TAGLIB */
#endif /* not T_FTGENERIC_H */

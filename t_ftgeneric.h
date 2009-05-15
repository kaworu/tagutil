#ifndef T_FTGENERIC_H
#define T_FTGENERIC_H
/*
 * t_ftgeneric.h
 *
 * a generic tagutil backend, using TagLib.
 */
#include "t_config.h"

#include "t_file.h"


__t__nonnull(1)
struct tfile * ftgeneric_new(const char *restrict path);

#endif /* not T_FTGENERIC_H */

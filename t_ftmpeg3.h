#ifndef T_FTMPEG3_H
#define T_FTMPEG3_H
/*
 * t_mpeg3.h
 *
 * MPEG layer 3 format handler with ID3Lib.
 */
#include "t_config.h"
#include "t_file.h"

#if defined(WITH_MPEG3)

void t_ftmpeg3_init(void);

_t__nonnull(1)
struct t_file * t_ftmpeg3_new(const char *restrict path);

#else /* not WITH_MPEG3 */

# define t_ftmpeg3_init() NOP
# define t_ftmpeg3_new(path) NULL

#endif /* WITH_MPEG3 */
#endif /* not T_FTMPEG3_H */

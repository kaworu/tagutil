#ifndef T_FTFLAC_H
#define T_FTFLAC_H
/*
 * t_ftflac.h
 *
 * FLAC format handler using libFLAC.
 */
#include "t_config.h"

#include "t_file.h"

#if defined(WITH_FLAC)

void ftflac_init(void);

__t__nonnull(1)
struct tfile * ftflac_new(const char *restrict path);

#else /* not WITH_FLAC */

# define ftflac_init()
# define ftflac_new(path) NULL

#endif /* WITH_FLAC */
#endif /* not T_FTFLAC_H */

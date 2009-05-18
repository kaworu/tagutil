#ifndef T_FTFLAC_H
#define T_FTFLAC_H
/*
 * t_ftflac.h
 *
 * FLAC format handler using libFLAC.
 */
#include "t_config.h"

#include "t_file.h"


__t__nonnull(1)
struct tfile * ftflac_new(const char *restrict path);

#endif /* not T_FTFLAC_H */

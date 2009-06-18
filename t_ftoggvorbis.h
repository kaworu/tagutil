#ifndef T_FTOGGVORBIS_H
#define T_FTOGGVORBIS_H
/*
 * t_ftoggvorbis.h
 *
 * Ogg/Vorbis backend, using libvorbis and libvorbisfile
 */
#include "t_config.h"
#include "t_file.h"

#if defined(WITH_OGGVORBIS)

void t_ftoggvorbis_init(void);

_t__nonnull(1)
struct t_file * t_ftoggvorbis_new(const char *restrict path);

#else /* not WITH_OGGVORBIS */

# define t_ftoggvorbis_init() (void)0
# define t_ftoggvorbis_new(path) NULL

#endif /* WITH_OGGVORBIS */
#endif /* not T_FTOGGVORBIS_H */

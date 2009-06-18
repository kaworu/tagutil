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

void ftoggvorbis_init(void);

_t__nonnull(1)
struct t_file * ftoggvorbis_new(const char *restrict path);

#else /* not WITH_OGGVORBIS */

# define ftoggvorbis_init()
# define ftoggvorbis_new(path) NULL

#endif /* WITH_OGGVORBIS */
#endif /* not T_FTOGGVORBIS_H */

#ifndef	T_FTOGGVORBIS_H
#define	T_FTOGGVORBIS_H
/*
 * t_ftoggvorbis.h
 *
 * Ogg/Vorbis backend, using libvorbis and libvorbisfile
 */
#include "t_config.h"
#include "t_file.h"

#if defined(WITH_OGGVORBIS)

extern t_file_ctor *t_ftoggvorbis_new;

#else /* not WITH_OGGVORBIS */
#	define	t_ftoggvorbis_new(path)	NULL
#endif /* WITH_OGGVORBIS */
#endif /* not T_FTOGGVORBIS_H */

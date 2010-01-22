#ifndef	T_FTFLAC_H
#define	T_FTFLAC_H
/*
 * t_ftflac.h
 *
 * FLAC format handler using libFLAC.
 */
#include "t_config.h"
#include "t_file.h"

#if defined(WITH_FLAC)

extern t_file_ctor *t_ftflac_new;

#else /* not WITH_FLAC */
#	define	t_ftflac_new(path)	NULL
#endif /* WITH_FLAC */
#endif /* not T_FTFLAC_H */

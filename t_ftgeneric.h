#ifndef	T_FTGENERIC_H
#define	T_FTGENERIC_H
/*
 * t_ftgeneric.h
 *
 * a generic tagutil backend, using TagLib.
 */
#include "t_config.h"
#include "t_file.h"


#if defined(WITH_TAGLIB)

extern t_file_ctor *t_ftgeneric_new;

#else /* not WITH_TAGLIB */
#	define t_ftgeneric_new(path)	NULL
#endif /* WITH_TAGLIB */
#endif /* not T_FTGENERIC_H */

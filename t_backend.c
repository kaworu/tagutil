/*
 * t_backend.c
 *
 * backends functions for tagutil
 */
#include "t_config.h"
#include "t_toolkit.h"
#include "t_backend.h"


_t__weak
struct t_backend *	t_flac_backend(void);
_t__weak
struct t_backend *	t_oggvorbis_backend(void);
_t__weak
struct t_backend *	t_generic_backend(void);


const struct t_backendL *
t_get_backend(void)
{
	static struct t_backendL L = SLIST_HEAD_INITIALIZER(L);

	if (SLIST_EMPTY(&L)) {
#if defined(WITH_TAGLIB)
		SLIST_INSERT_HEAD(&L, t_generic_backend(), next);
#endif
#if defined(WITH_OGGVORBIS)
		SLIST_INSERT_HEAD(&L, t_oggvorbis_backend(), next);
#endif
#if defined(WITH_FLAC)
		SLIST_INSERT_HEAD(&L, t_flac_backend(), next);
#endif
	}
	return (&L);
}


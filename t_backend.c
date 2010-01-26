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


const struct t_backendQ *
t_get_backend(void)
{
	static struct t_backendQ bQ = TAILQ_HEAD_INITIALIZER(bQ);

	if (TAILQ_EMPTY(&bQ)) {
#if defined(WITH_TAGLIB)
		TAILQ_INSERT_HEAD(&bQ, t_generic_backend(), next);
#endif
#if defined(WITH_OGGVORBIS)
		TAILQ_INSERT_HEAD(&bQ, t_oggvorbis_backend(), next);
#endif
#if defined(WITH_FLAC)
		TAILQ_INSERT_HEAD(&bQ, t_flac_backend(), next);
#endif
	}
	return (&bQ);
}


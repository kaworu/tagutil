/*
 * t_backend.c
 *
 * backends functions for tagutil
 */
#include "t_config.h"
#include "t_toolkit.h"

#include "t_backend.h"


struct t_backend	*t_ftflac_backend(void) t__weak;
struct t_backend	*t_ftoggvorbis_backend(void) t__weak;
struct t_backend	*t_ftgeneric_backend(void) t__weak;


const struct t_backendQ *
t_all_backends(void)
{
	static int initialized = 0;
	static struct t_backendQ bQ = TAILQ_HEAD_INITIALIZER(bQ);

	if (!initialized) {
		/* add each available backend (order matter) */
		if (t_ftflac_backend != NULL)
			TAILQ_INSERT_TAIL(&bQ, t_ftflac_backend(), entries);

		if (t_ftoggvorbis_backend != NULL)
			TAILQ_INSERT_TAIL(&bQ, t_ftoggvorbis_backend(), entries);

		if (t_ftgeneric_backend != NULL)
			TAILQ_INSERT_TAIL(&bQ, t_ftgeneric_backend(), entries);

		initialized = 1;
	}

	return (&bQ);
}

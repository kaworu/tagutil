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
struct t_backend	*t_fttaglib_backend(void) t__weak;
struct t_backend	*t_ftid3v1_backend(void) t__weak;


const struct t_backendQ *
t_all_backends(void)
{
	static int initialized = 0;
	static struct t_backendQ bQ = TAILQ_HEAD_INITIALIZER(bQ);

	if (!initialized) {
		/* add each available backend (order matter) */

		/* FLAC files support using libflac */
		if (t_ftflac_backend != NULL)
			TAILQ_INSERT_TAIL(&bQ, t_ftflac_backend(), entries);

		/* Ogg/Vorbis files support using libogg/libvorbis */
		if (t_ftoggvorbis_backend != NULL)
			TAILQ_INSERT_TAIL(&bQ, t_ftoggvorbis_backend(), entries);

		/* Multiple files types support using TagLib */
		if (t_fttaglib_backend != NULL)
			TAILQ_INSERT_TAIL(&bQ, t_fttaglib_backend(), entries);

		/* mp3 ID3v1.1 files types support */
		if (t_ftid3v1_backend != NULL)
			TAILQ_INSERT_TAIL(&bQ, t_ftid3v1_backend(), entries);

		initialized = 1;
	}

	return (&bQ);
}

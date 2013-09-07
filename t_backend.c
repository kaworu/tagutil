/*
 * t_backend.c
 *
 * backends functions for tagutil
 */
#include "t_config.h"
#include "t_toolkit.h"
#include "t_backend.h"


t__weak
struct t_backend *t_ftflac_backend(void);
t__weak
struct t_backend *t_ftoggvorbis_backend(void);
t__weak
struct t_backend *t_generic_backend(void);


const struct t_backendQ *
t_all_backends(void)
{
	static int initialized = 0;
	static struct t_backendQ bQ = TAILQ_HEAD_INITIALIZER(bQ);

	if (!initialized) {
		struct t_backend *b, *b_temp;
		char	*env;

		/* add each available backend */
#if defined(WITH_TAGLIB)
		TAILQ_INSERT_HEAD(&bQ, t_generic_backend(), entries);
#endif
#if defined(WITH_OGGVORBIS)
		TAILQ_INSERT_HEAD(&bQ, t_ftoggvorbis_backend(), entries);
#endif
#if defined(WITH_FLAC)
		TAILQ_INSERT_HEAD(&bQ, t_ftflac_backend(), entries);
#endif
		/* change backend priority if requested by the env */
		env = getenv("TAGUTIL_BACKEND");
		if (env != NULL) {
			char *start, *cur, *next;
			struct t_backend *target;

			next = start = strdup(env);
			while (next != NULL) {
				next = strrchr(start, ',');
				if (next == NULL)
					cur = start;
				else {
					cur = next + 1;
					*next = '\0';
				}
				target = NULL;
				TAILQ_FOREACH(b, &bQ, entries) {
					if (strcasecmp(b->libid, cur) == 0) {
						target = b;
						break;
					}
				}
				if (target != NULL) {
					TAILQ_REMOVE(&bQ, target, entries);
					TAILQ_INSERT_HEAD(&bQ, target, entries);
				}
			}
			free(start);
		}

		/* call setup routine foreach backend needing it. */
		TAILQ_FOREACH_SAFE(b, &bQ, entries, b_temp) {
			if (b->setup != NULL) {
				if (b->setup() != 0) {
					warnx("backend %s setup failed.", b->libid);
					TAILQ_REMOVE(&bQ, b, entries);
				}
			}
		}

		initialized = 1;
	}
	return (&bQ);
}

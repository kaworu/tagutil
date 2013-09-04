/*
 * t_backend.c
 *
 * backends functions for tagutil
 */
#include "t_config.h"
#include "t_toolkit.h"
#include "t_backend.h"


t__weak
struct t_backend *t_flac_backend(void);
t__weak
struct t_backend *t_oggvorbis_backend(void);
t__weak
struct t_backend *t_generic_backend(void);


const struct t_backendQ *
t_all_backends(void)
{
	static struct t_backendQ bQ = TAILQ_HEAD_INITIALIZER(bQ);

	if (TAILQ_EMPTY(&bQ)) {
		char	*env;

#define	MAX_BACKEND	3
#if defined(WITH_TAGLIB)
		TAILQ_INSERT_HEAD(&bQ, t_generic_backend(), entries);
#endif
#if defined(WITH_OGGVORBIS)
		TAILQ_INSERT_HEAD(&bQ, t_oggvorbis_backend(), entries);
#endif
#if defined(WITH_FLAC)
		TAILQ_INSERT_HEAD(&bQ, t_flac_backend(), entries);
#endif

		env = getenv("TAGUTIL_BACKEND");
		if (env != NULL) {
			char	*start, *cur, *next;
			struct t_backend	*b, *target;

			next = start = xstrdup(env);
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
	}
	return (&bQ);
}


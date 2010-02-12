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
		char	*env;

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
			int	i;
			char	*s;
			char	**bv, **bp;

			env = xstrdup(env);
			s = env;
			for (i = 0; s != NULL; i++) {
				s++;
				s = strchr(s, ',');
			}
			i++; /* last NULL element */
			bv = xcalloc(i, sizeof(char *));
			bv[--i] = NULL;

			s = env;
			while (i > 0) {
				assert_not_null(s);
				bv[--i] = s;
				s = strchr(s, ',');
				if (s != NULL) {
					*s = '\0';
					s++;
				}
			}

			for (bp = bv; *bp != NULL; bp++) {
				struct t_backend *b, *target;
				target = NULL;
				TAILQ_FOREACH(b, &bQ, entries) {
					if (strcasecmp(b->libid, *bp) == 0) {
						target = b;
						break;
					}
				}
				if (target != NULL) {
					TAILQ_REMOVE(&bQ, target, entries);
					TAILQ_INSERT_HEAD(&bQ, target, entries);
				}
			}
			free(bv);
			free(env);
		}
	}
	return (&bQ);
}


#ifndef T_BACKEND_H
#define T_BACKEND_H
/*
 * t_backend.h
 *
 * backends functions for tagutil
 */
#include "t_config.h"
#include "t_file.h"


struct t_backend {
	const char	*libid;
	const char	*desc;
	t_file_ctor	*ctor;

	TAILQ_ENTRY(t_backend)	entries;
};
TAILQ_HEAD(t_backendQ, t_backend);

/*
 * initialize backends if needed and return the list
 */
const struct t_backendQ * t_all_backends(void);

#endif /* not T_BACKEND_H */

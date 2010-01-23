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

	SLIST_ENTRY(t_backend)	next;
};
SLIST_HEAD(t_backendL, t_backend);


/*
 * initialize backends if needed and return the list
 */
const struct t_backendL * t_get_backend(void);

#endif /* not T_BACKEND_H */

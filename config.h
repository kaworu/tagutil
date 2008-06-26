#ifndef T_CONFIG_H
#define T_CONFIG_H


#define __TAGUTIL_VERSION__ "1.1"
#define __TAGUTIL_AUTHOR__ "kAworu"


/* use gcc attribute when available */
#if !defined(__GNUC__)
#  define __attribute__(x)
#endif
/* avoid lint to complain for non C89 keywords */
#if defined(lint)
#define inline
#define restrict
#endif

/* OS config */

/*
 * dirname(3)
 * Some implementations define a const char * as argument, which is a *sane* dirname.
 */
#if !defined(__FreeBSD__) && !defined(__OpenBSD__)
#define WITH_INSANE_DIRNAME
#endif /* !__FreeBSD__ && !__OpenBSD__ */

/* APP config */
#define WITH_MB
#define MB_DEFAULT_HOST "localhost"
#define MB_DEFAULT_PORT "80"

#endif /* !T_CONFIG_H */

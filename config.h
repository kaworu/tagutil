#ifndef T_CONFIG_H
#define T_CONFIG_H


#define __TAGUTIL_VERSION__ "1.1"
#define __TAGUTIL_AUTHOR__ "kAworu"


#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L /* C99 */

#include <stdbool.h>
#define __inline__ inline
#define __restrict__ restrict
#define __FUNCNAME__ __func__

#else /* !C99 */

typedef unsigned char bool;
#define false (/*CONSTCOND*/0)
#define true (!false)
#define __inline__
#define __restrict__
#if defined(__GNUC__)
#define __FUNCNAME__ __FUNCTION__
#else /* !__GNUC__ */
#define __FUNCNAME__ ""
#endif /* __GNUC__ */

#endif /* C99 */


/* use gcc attribute when available */
#if !defined(__GNUC__) && !defined(__attribute__)
#  define __attribute__(x)
#endif


/* APP config */
#define WITH_MB


#if defined(WITH_MB)
#define MB_DEFAULT_HOST "localhost"
#define MB_DEFAULT_PORT "80"
#endif
#endif /* !T_CONFIG_H */

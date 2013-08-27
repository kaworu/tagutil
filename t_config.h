#ifndef	T_CONFIG_H
#define	T_CONFIG_H

#if !defined(T_TAGUTIL_VERSION)
#	define	T_TAGUTIL_VERSION "(unknow version)"
#endif /* not T_TAGUTIL_VERSION */

/*
 * avoid lint to complain for non C89 keywords and macros
 */
#if defined(lint)
#	define	inline
#	define	_t__weak
#	define	_t__unused
#	define	_t__dead2
#	define	_t__nonnull(x)
#	define	_t__printflike(fmtarg, firstvarg)
#else
#	define	_t__weak	__attribute__((__weak__))
#	define	_t__unused	__attribute__((__unused__))
#	define	_t__dead2	__attribute__((__noreturn__))
#	define	_t__nonnull(x)	__attribute__((__nonnull__(x)))
#	define	_t__printflike(fmtarg, firstvararg) \
		    __attribute__((__format__(__printf__, fmtarg, firstvararg)))
#endif /* lint */

#include "compat/include/sys/queue.h"
#if !defined(HAS_GETPROGNAME)
#	include "compat/include/getprogname.h"
#endif
#if !defined(HAS_STRLCPY)
#	include "compat/include/strlcpy.h"
#endif
#if !defined(HAS_STRLCAT)
#	include "compat/include/strlcat.h"
#endif
#if !defined(HAS_STRDUP)
#	include "compat/include/strdup.h"
#endif
#if !defined(HAS_SBUF)
#	include <sys/types.h>
#	include "compat/include/sys/sbuf.h"
#else
#	include <sys/types.h>
#	include <sys/sbuf.h>
#endif

#endif /* not T_CONFIG_H */

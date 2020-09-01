#ifndef	T_CONFIG_H
#define	T_CONFIG_H

#if !defined(T_TAGUTIL_VERSION)
#	define	T_TAGUTIL_VERSION "(unknow version)"
#endif /* ndef T_TAGUTIL_VERSION */

/*
 * avoid lint to complain for non C89 keywords and macros
 */
#if defined(lint)
#	define	inline
#	define	t__weak
#	define	t__packed
#	define	t__unused
#	define	t__dead2
#	define	t__deprecated
#	define	t__printflike(fmtarg, firstvarg)
#else
#	define	t__weak		__attribute__((__weak__))
#	define	t__packed	__attribute__((__packed__))
#	define	t__unused	__attribute__((__unused__))
#	define	t__dead2	__attribute__((__noreturn__))
#	define	t__deprecated	__attribute__((__deprecated__))
#	define	t__printflike(fmtarg, firstvararg) \
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

/*
 * macOS is the only known platform to provide these alternatives names to POSIX
 * st_mtim members.
 */
#if defined(__APPLE__)
#	define st_mtim st_mtimespec
#endif

#endif /* ndef T_CONFIG_H */

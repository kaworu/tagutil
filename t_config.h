#ifndef T_CONFIG_H
#define T_CONFIG_H

#if !defined(VERSION)
#  define VERSION "(unknow version)"
#endif /* not VERSION */

/*
 * avoid lint to complain for non C89 keywords and macros
 */
#if defined(lint)
#  define inline
#  define restrict
#  define __t__unused
#  define __t__nonnull(x)
#  define __t__dead2
#  define __t__printflike(fmtarg, firstvarg)
#else
#include <sys/cdefs.h>
#  define __t__unused     __unused
#  define __t__dead2      __dead2
#  define __t__nonnull(x) __nonnull(x)
#  define __t__printflike(fmtarg, firstvarg) __printflike(fmtarg, firstvarg)
#endif /* lint */


#endif /* not T_CONFIG_H */

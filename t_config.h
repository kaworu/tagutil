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
#	define	restrict
#	define	_t__unused
#	define	_t__dead2
#	define	_t__nonnull(x)
#	define	_t__printflike(fmtarg, firstvarg)
#else
#	define	_t__unused	__attribute__((unused))
#	define	_t__dead2	__attribute__((noreturn))
#	define	_t__nonnull(x)	__attribute__((nonnull(x)))
#	define	_t__printflike(fmtarg, firstvararg) \
		    __attribute__((format(printf, fmtarg, firstvararg)))
#endif /* lint */


#endif /* not T_CONFIG_H */

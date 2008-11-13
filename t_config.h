#ifndef T_CONFIG_H
#define T_CONFIG_H


#define __TAGUTIL_VERSION__ "2.0_pre1"
#define __TAGUTIL_AUTHOR__ "kAworu"

/* avoid lint to complain for non C89 keywords */
#if defined(lint)
#  define inline
#  define restrict
#endif /* lint */

#if defined(T_NO_DEV)
#  define __t__unused
#  define __t__nonnull(x)
#  define __t__dead2
#else
#  define __t__unused     __unused
#  define __t__nonnull(x) __nonnull(x)
#  define __t__dead2      __dead2
#endif /* T_NO_DEV */

#endif /* not T_CONFIG_H */

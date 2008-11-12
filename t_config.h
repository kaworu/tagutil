#ifndef T_CONFIG_H
#define T_CONFIG_H


#define __TAGUTIL_VERSION__ "2.0_pre1"
#define __TAGUTIL_AUTHOR__ "kAworu"

/* avoid lint to complain for non C89 keywords */
#if defined(lint)
#  define inline
#  define restrict
#endif /* lint */

#endif /* not T_CONFIG_H */

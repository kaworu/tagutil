/*
 * compat.c
 * a compat file for tagutil.
 */

#include "t_config.h"


#if !defined(HAS_GETPROGNAME)
const char *
getprogname(void)
{
    extern char *__progname;

    return (__progname);
}
#endif

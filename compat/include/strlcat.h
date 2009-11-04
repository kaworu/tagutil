#ifndef T_COMPAT_STRLCAT_H
#define T_COMPAT_STRLCAT_H
/*
 * OpenBSD's strlcpy(3) and strlcat(3),
 *      consistent, safe, string copy and concatenation.
 */
#include <stdlib.h>


size_t strlcat(char *dst, const char *src, size_t siz);

#endif /* not T_COMPAT_STRLCAT_H */

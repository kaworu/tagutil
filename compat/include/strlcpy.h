#ifndef T_COMPAT_STRLCPY_H
#define T_COMPAT_STRLCPY_H
/*
 * OpenBSD's strlcpy(3) and strlcat(3),
 *      consistent, safe, string copy and concatenation.
 */
#include <stdlib.h>


size_t strlcpy(char *dst, const char *src, size_t siz);

#endif /* not T_COMPAT_STRLCPY_H */

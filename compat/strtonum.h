#ifndef T_COMPAT_STRTONUM_H
#define T_COMPAT_STRTONUM_H
/*
 * OpenBSD's strtonum(3),
 *      designed to facilitate safe, robust program-ming and overcome the
 *      shortcomings of the atoi(3) and strtol(3) family of interfaces.
 */
long long strtonum(const char *numstr, long long minval,
        long long maxval, const char **errstrp);

#endif /* not T_COMPAT_STRTONUM_H */

#ifndef T_LOADER_H
#define T_LOADER_H
/*
 * t_loader.h
 *
 * Routine to load a YAML tag file in a tune for tagutil.
 */
#include "t_tune.h"

/*
 * parse a given fmtfile and set the given tune's tags accordingly.
 *
 * @param tune
 *   The tune to load the parsed tags into.
 *
 * @param fmtfile
 *   The path of a file containing tags to parse. if `-' is given, stdin is
 *   used.
 *
 * @return
 *   -1 on error, 0 on success.
 */
int	t_load(struct t_tune *tune, const char *fmtfile);

#endif /* ndef T_LOADER_H */

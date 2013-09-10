#ifndef	T_INTERPRETER_H
#define	T_INTERPRETER_H
/*
 * t_filter.h
 *
 * the tagutil's filter interpreter.
 */
#include <stdbool.h>

#include "t_config.h"
#include "t_tune.h"
#include "t_parser.h"


/*
 * eval the given filter in the context of file.
 *
 * return true if file match the filter, false otherwise.
 */
bool	t_filter_eval(struct t_tune *tune, const struct t_ast *filter);

#endif /* not T_INTERPRETER_H */

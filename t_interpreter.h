#ifndef T_INTERPRETER_H
#define T_INTERPRETER_H
/*
 * t_interpreter.h
 *
 * the tagutil's filter interpreter.
 */
#include <stdbool.h>

#include "t_config.h"
#include "t_file.h"
#include "t_parser.h"


/*
 * TODO
 */
_t__nonnull(1) _t__nonnull(2)
bool t_interpreter_eval_ast(const struct t_file *restrict file,
        const struct t_ast *restrict filter);

#endif /* not T_INTERPRETER_H */

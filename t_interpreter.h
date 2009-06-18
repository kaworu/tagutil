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


_t__nonnull(1) _t__nonnull(2)
bool ast_eval(const struct tfile *restrict file,
        const struct ast *restrict filter);

#endif /* not T_INTERPRETER_H */

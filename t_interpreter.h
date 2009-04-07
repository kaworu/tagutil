#ifndef T_INTERPRETER_H
#define T_INTERPRETER_H
/*
 * t_interpreter.c
 *
 * the tagutil's filter interpreter.
 */
#include "t_config.h"

#include <stdbool.h>

#include "t_file.h"
#include "t_parser.h"


#define is_int_tkeyword(t) ((t) == TTRACK || (t) == TYEAR)


__t__nonnull(1) __t__nonnull(2)
bool eval(struct tfile *restrict file,
        const struct ast *restrict filter);

#endif /* not T_INTERPRETER_H */

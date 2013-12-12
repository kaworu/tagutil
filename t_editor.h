#ifndef T_EDITOR_H
#define T_EDITOR_H
/*
 * t_editor.h
 *
 * editor routines for tagutil.
 */
#include "t_tune.h"

/*
 * Save the given tune's tags (YAML) in a temporary file, fork(2) to call
 * $EDITOR and then load the temporary file in the tune.
 *
 * @param tune
 *   The tune to edit.
 *
 * @return
 *  -1 on error, 0 on success.
 */
int	t_edit(struct t_tune *tune);

#endif /* ndef T_EDITOR_H */

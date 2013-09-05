#ifndef T_FILE_COMPAT_H
#define T_FILE_COMPAT_H
/*
 * t_file_compat.h
 *
 * temporary wrapper of t_file around t_tune.
 */
#include "t_config.h"
#include "t_backend.h"
#include "t_file.h"
#include "t_tune.h"

struct t_file		*t_file_new(const char *path) t__deprecated;
void			 t_file_destroy(struct t_file *file) t__deprecated;
bool			 t_file_save(struct t_file *file) t__deprecated;
struct t_taglist	*t_file_get(struct t_file *file, const char *key) t__deprecated;
bool			 t_file_clear(struct t_file *file, const struct t_taglist *T) t__deprecated;
bool			 t_file_add(struct t_file *file, const struct t_taglist *T) t__deprecated;

#endif /* ndef T_FILE_COMPAT_H */

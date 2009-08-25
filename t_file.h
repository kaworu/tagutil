#ifndef T_FILE_H
#define T_FILE_H
/*
 * t_file.h
 *
 * tagutil Tag File
 *
 * You can rely on the fact that this header include the t_tag.h and t_error.h
 * header.
 */
#include <stdlib.h>
#include <stdbool.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_error.h"
#include "t_tag.h"



/* abstract music file, with method members */
struct t_file {
	const char	*path;
	const char	*lib;
	void		*data;

	/*
	 * constructor.
	 *
	 * return NULL if it couldn't create the struct.
	 * returned value has to be free()d (use file->destroy(file)).
	 */
	struct t_file * (*new)(const char *restrict path);

	/*
	 * write the file.
	 *
	 * return true if no error, false otherwise.
	 * On success t_error_msg(file) is NULL, otherwise it contains an error
	 * message.
	 */
	bool (*save)(struct t_file *restrict file);

	/*
	 * free the struct
	 */
	void (*destroy)(struct t_file *restrict file);

	/*
	 * return a the values of the tag key.
	 *
	 * If key is NULL, all the tags are returned.
	 * If there is no values fo key, ret->tcount is 0.
	 *
	 * On error, NULL is returned and t_error_msg(file) contains an error
	 * message, otherwise t_error_msg(file) is NULL.
	 *
	 * returned value has to be free()d (use t_taglist_destroy()).
	 */
	struct t_taglist * (*get)(struct t_file *restrict file,
	    const char *restrict key);

	/*
	 * clear the given key tag (all values).
	 *
	 * if T is NULL, all tags will be cleared.
	 *
	 * On success true is returned, otherwise false is returned and
	 * t_error_msg(file) contains an error message.
	 */
	bool (*clear)(struct t_file *restrict file,
	    const struct t_taglist *restrict T);

	/*
	 * add the tags of given t_taglist in file.
	 *
	 * On success true is returned, otherwise false is returned and
	 * t_error_msg(file) contains an error message.
	 */
	bool (*add)(struct t_file *restrict file,
	    const struct t_taglist *restrict T);


	T_ERROR_MSG_MEMBER;
};


/*
 * t_file member function setter.
 */
#define T_FILE_FUNC_INIT(file, libid)						\
	do {									\
		(file)->new = T_CONCAT(T_CONCAT(t_ft, libid), _new);		\
		(file)->save = T_CONCAT(T_CONCAT(t_ft, libid), _save);		\
		(file)->destroy = T_CONCAT(T_CONCAT(t_ft, libid), _destroy);	\
		(file)->get = T_CONCAT(T_CONCAT(t_ft, libid), _get);		\
		(file)->clear = T_CONCAT(T_CONCAT(t_ft, libid), _clear);	\
		(file)->add = T_CONCAT(T_CONCAT(t_ft, libid), _add);		\
	} while (/*CONSTCOND*/0)


/*
 * generic t_file constructor
 */
_t__unused _t__nonnull(1)
static struct t_file * t_file_new(const char *restrict path,
    const char *restrict libname,
    const void *restrict data, size_t datalen);


static struct t_file *
t_file_new(const char *restrict path,
    const char *restrict libname,
    const void *restrict data, size_t datalen)
{
	char *s;
	size_t pathsize;
	struct t_file *file;

	assert_not_null(path);

	pathsize = strlen(path) + 1;
	file = xmalloc(sizeof(struct t_file) + datalen + pathsize);

	file->data = (void *)(file + 1);
	(void)memcpy(file->data, data, datalen);

	s = (char *)((char *)(file->data) + datalen);
	assert(strlcpy(s, path, pathsize) < pathsize);
	file->path = s;

	file->lib = libname;

	t_error_init(file);
	return (file);
}

#endif /* not T_FILE_H */

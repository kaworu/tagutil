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
 * t_file ctor helper.
 */
#define T_FILE_NEW(file, path, data)						\
	do {									\
		(file) = xmalloc(sizeof(*(file)) + sizeof(data) +		\
		    strlen(path) + 1);						\
		(file)->data = (void *)((file) + 1);				\
		(void)memcpy((file)->data, &data, sizeof(data));		\
		(file)->path = (char *)(file)->data + sizeof(data);		\
		(void)memcpy((char *)(file)->data + sizeof(data),		\
		    path, strlen(path) + 1);					\
		(file)->lib = libname;						\
		t_error_init(file);						\
		(file)->new = t_file_new;					\
		(file)->save = t_file_save;					\
		(file)->destroy = t_file_destroy;				\
		(file)->get = t_file_get;					\
		(file)->clear = t_file_clear;					\
		(file)->add = t_file_add;					\
	} while (/*CONSTCOND*/0);


/*
 * t_file ctor type
 */
typedef struct t_file * t_file_ctor(const char *restrict path);

#endif /* not T_FILE_H */

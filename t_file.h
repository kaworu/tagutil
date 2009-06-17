#ifndef T_FILE_H
#define T_FILE_H
/*
 * t_file.h
 *
 * tagutil Tag File
 * You can rely on the fact that this header include the t_tag.h header.
 */
#include "t_config.h"

#include "t_tag.h"

#include <stdbool.h>


/* abstract music file, with method members */
struct tfile {
    const char *path;
    const char *lib;
    void *data;

    /*
     * constructor.
     *
     * return NULL if it couldn't create the struct.
     * returned value has to be free()d (use file->destroy(file)).
     */
    struct tfile * (*create)(const char *restrict path);

    /*
     * write the file.
     *
     * return true if no error, false otherwise.
     * On success t_error_msg(self) is NULL, otherwise it contains an error
     * message.
     */
    bool (*save)(struct tfile *restrict self);

    /*
     * free the struct
     */
    void (*destroy)(struct tfile *restrict self);

    /*
     * return a the values of the tag key.
     *
     * If key is NULL, all the tags are returned.
     * If there is no values fo key, ret->tcount is 0.
     *
     * On error, NULL is returned and t_error_msg(self) contains an error
     * message, otherwise t_error_msg(self) is NULL.
     *
     * returned value has to be free()d (use destroy_tag_list()).
     */
    struct tag_list * (*get)(struct tfile *restrict self,
            const char *restrict key);

    /*
     * clear the given key tag (all values).
     *
     * if T is NULL, all tags will be cleared.
     *
     * On success true is returned, otherwise false is returned and
     * t_error_msg(self) contains an error message.
     */
    bool (*clear)(struct tfile *restrict self, const struct tag_list *restrict T);

    /*
     * add the tags of given tag_list in self.
     *
     * On success true is returned, otherwise false is returned and
     * t_error_msg(self) contains an error message.
     */
    bool (*add)(struct tfile *restrict self, const struct tag_list *restrict T);


    ERROR_MSG_MEMBER;
};

#endif /* not T_FILE_H */

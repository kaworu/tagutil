#ifndef T_FILE_H
#define T_FILE_H
/*
 * t_file.h
 *
 * tagutil Tag File
 */
#include "t_config.h"

#include <stdbool.h>


struct tfile {
    const char *path;
    const char *lib;
    void *data;

    /*
     * constructor, return NULL if it couldn't create the struct.
     */
    struct tfile * (*create)(const char *restrict path);

    /*
     * save the file, return true if no error, false otherwise.
     */
    bool (*save)(struct tfile *restrict self);

    /*
     * free the struct
     */
    void (*destroy)(struct tfile *restrict self);

    /*
     * return a the value of the tag "key". If there isn't any, or it's not
     * supported it return NULL.
     *
     * returned value has to be freed.
     */
    char * (*get)(const struct tfile *restrict self, const char *restrict key);

    /*
     * set the tag key to value newval. return 0 if no errors.
     * XXX: do a enum.
     */
    int (*set)(struct tfile *restrict self, const char *restrict key,
            const char *restrict newval);

    /*
     * return the number of tags set.
     */
    int (*tagcount)(const struct tfile *restrict self);

    /*
     * return an array of the tag key that are set.
     *
     * returned value has to be freed.
     */
    char ** (*tagkeys)(const struct tfile *restrict self);
};

#endif /* not T_FILE_H */

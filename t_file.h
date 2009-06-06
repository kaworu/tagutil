#ifndef T_FILE_H
#define T_FILE_H
/*
 * t_file.h
 *
 * tagutil Tag File
 */
#include "t_config.h"

#include <stdbool.h>

/* set() return values */
enum tfile_set_status {
    TFILE_SET_STATUS_OK,
    TFILE_SET_STATUS_BADARG,
    TFILE_SET_STATUS_LIBERROR
};


struct tfile {
    const char *path;
    const char *lib;
    void *data;

    /*
     * constructor, return NULL if it couldn't create the struct.
     *
     * returned value has to be free()d (use file->destroy(file)).
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
     * returned value has to be free()d.
     */
    char * (*get)(const struct tfile *restrict self, const char *restrict key);

    /*
     * set the tag key to value newval.
     * if newval == NULL, the tag is deleted (provided that the backend support
     * tag deletion).
     */
    enum tfile_set_status (*set)(struct tfile *restrict self,
            const char *restrict key, const char *restrict newval);

    /*
     * store an array of the tag key that are set in kptr. return -1 if error,
     * or the count of tag keys in kptr otherwise.
     *
     * stored values (in kptr) have to be free()d.
     */
    long (*tagkeys)(const struct tfile *restrict self, char ***kptr);
};

#endif /* not T_FILE_H */

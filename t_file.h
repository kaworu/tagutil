#ifndef T_FILE_H
#define T_FILE_H
/*
 * t_file.h
 *
 * tagutil Tag File
 */
#include "t_config.h"


struct tfile {
    const char *path;
    void *data;

    struct tfile * (*create)(const char *restrict path);
    int (*save)(struct tfile *restrict self);
    int (*destroy)(struct tfile *restrict self);

    const char * (*get)(const struct tfile *restrict self, const char *restrict key);
    int (*set)(struct tfile *restrict self, const char *restrict key,
            const char *restrict newval);

    const char ** (*tagkeys)(const struct tfile *restrict self);
};

#endif /* not T_FILE_H */

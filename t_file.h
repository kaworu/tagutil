#ifndef T_FILE_H
#define T_FILE_H
/*
 * t_file.h
 *
 * tagutil Tag File
 */
#include "t_config.h"

#include <sys/cdefs.h> /* __CONCAT macro */

struct tfile;
typedef const char * (*sgetter_f)(struct tfile *self);
typedef int (*ssetter_f)(struct tfile *self, const char *newval);
typedef unsigned int (*igetter_f)(struct tfile *self);
typedef int (*isetter_f)(struct tfile *self, unsigned int newval);

struct tfile {
    const char *path;

    struct tfile * (*create)(const char *path);
    int (*save)(struct tfile *self);
    int (*destroy)(struct tfile *self);

    sgetter_f artist, album, comment, genre, title;
    ssetter_f set_artist, set_album, set_comment, set_genre, set_title;
    igetter_f track, year;
    isetter_f set_track, set_year;

    struct {
    void *data;
    char *artist, *album, *comment, *genre, *title;
    unsigned int track, year;
    } _private;
};


#define TFILE_MEMBER_F(prefix,tf) do {                 \
    (tf)->create      = __CONCAT(prefix,_new);         \
    (tf)->save        = __CONCAT(prefix,_save);        \
    (tf)->destroy     = __CONCAT(prefix,_destroy);     \
    (tf)->artist      = __CONCAT(prefix,_artist);      \
    (tf)->album       = __CONCAT(prefix,_album);       \
    (tf)->comment     = __CONCAT(prefix,_comment);     \
    (tf)->genre       = __CONCAT(prefix,_genre);       \
    (tf)->title       = __CONCAT(prefix,_title);       \
    (tf)->track       = __CONCAT(prefix,_track);       \
    (tf)->year        = __CONCAT(prefix,_year);        \
    (tf)->set_artist  = __CONCAT(prefix,_set_artist);  \
    (tf)->set_album   = __CONCAT(prefix,_set_album);   \
    (tf)->set_comment = __CONCAT(prefix,_set_comment); \
    (tf)->set_genre   = __CONCAT(prefix,_set_genre);   \
    (tf)->set_title   = __CONCAT(prefix,_set_title);   \
    (tf)->set_track   = __CONCAT(prefix,_set_track);   \
    (tf)->set_year    = __CONCAT(prefix,_set_year);    \
} while (/*CONSTCOND*/0)

#endif /* not T_FILE_H */

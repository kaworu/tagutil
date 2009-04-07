/*
 * t_ftgeneric.c
 *
 * a generic tagutil backend, using TagLib.
 */
#include "t_config.h"

#include <tag_c.h>

#include "t_file.h"
#include "t_ftgeneric.h"
#include "t_toolkit.h"


#define T_TAGLIB_SGETTER_HOOK(X)                         \
__t__nonnull(1)                                          \
const char *                                             \
__CONCAT(ftgeneric_,X)(struct tfile *self)               \
{                                                        \
    assert_not_null(self);                               \
    assert_not_null(self->_private.data);                \
    if (self->_private.X == NULL) {                      \
        char *s;                                         \
        TagLib_File *f = self->_private.data;            \
        s = __CONCAT(taglib_tag_,X)(taglib_file_tag(f)); \
        self->_private.X = xstrdup(s);                   \
    }                                                    \
    return (self->_private.X);                           \
}
T_TAGLIB_SGETTER_HOOK(artist)
T_TAGLIB_SGETTER_HOOK(album)
T_TAGLIB_SGETTER_HOOK(comment)
T_TAGLIB_SGETTER_HOOK(genre)
T_TAGLIB_SGETTER_HOOK(title)


#define T_TAGLIB_IGETTER_HOOK(X)                         \
__t__nonnull(1)                                          \
unsigned int                                             \
__CONCAT(ftgeneric_,X)(struct tfile *self)               \
{                                                        \
    unsigned int i;                                      \
    assert_not_null(self);                               \
    assert_not_null(self->_private.data);                \
    if (self->_private.X == 0) {                         \
        TagLib_File *f = self->_private.data;            \
        i = __CONCAT(taglib_tag_,X)(taglib_file_tag(f)); \
        self->_private.X = i;                            \
    }                                                    \
    return (self->_private.X);                           \
}
T_TAGLIB_IGETTER_HOOK(track)
T_TAGLIB_IGETTER_HOOK(year)


#define T_TAGLIB_SSETTER_HOOK(X)                             \
__t__nonnull(1) __t__nonnull(2)                              \
int                                                          \
__CONCAT(ftgeneric_set_,X)(struct tfile *self,               \
    const char *newval)                                      \
{                                                            \
    TagLib_File *f;                                          \
    assert_not_null(self);                                   \
    assert_not_null(self->_private.data);                    \
    assert_not_null(newval);                                 \
    f = self->_private.data;                                 \
    __CONCAT(taglib_tag_set_,X)(taglib_file_tag(f), newval); \
    if (self->_private.X != NULL)                            \
        free(self->_private.X);                              \
    self->_private.X = xstrdup(newval);                      \
    return (0);                                              \
}
T_TAGLIB_SSETTER_HOOK(artist)
T_TAGLIB_SSETTER_HOOK(album)
T_TAGLIB_SSETTER_HOOK(comment)
T_TAGLIB_SSETTER_HOOK(genre)
T_TAGLIB_SSETTER_HOOK(title)


#define T_TAGLIB_ISETTER_HOOK(X)                             \
__t__nonnull(1)                                              \
int                                                          \
__CONCAT(ftgeneric_set_,X)(struct tfile *self,               \
    unsigned int newval)                                     \
{                                                            \
    TagLib_File *f;                                          \
    assert_not_null(self);                                   \
    assert_not_null(self->_private.data);                    \
    f = self->_private.data;                                 \
    __CONCAT(taglib_tag_set_,X)(taglib_file_tag(f), newval); \
    self->_private.X = newval;                               \
    return (0);                                              \
}
T_TAGLIB_ISETTER_HOOK(track)
T_TAGLIB_ISETTER_HOOK(year)


__t__nonnull(1)
int
ftgeneric_destroy(struct tfile *self)
{
    TagLib_File *f;

    assert_not_null(self);
    assert_not_null(self->_private.data);

    f = self->_private.data;
    taglib_file_free(f);

    if (self->_private.artist != NULL)
        free(self->_private.artist);
    if (self->_private.album != NULL)
        free(self->_private.album);
    if (self->_private.comment != NULL)
        free(self->_private.comment);
    if (self->_private.genre != NULL)
        free(self->_private.genre);
    if (self->_private.title != NULL)
        free(self->_private.title);

    free(self);
    return (0);
}


__t__nonnull(1)
int
ftgeneric_save(struct tfile *self)
{
    TagLib_File *f;

    assert_not_null(self);
    assert_not_null(self->_private.data);

    f = self->_private.data;

    if (taglib_file_save(f))
        return (0);
    else
        return (1);
}


struct tfile *
ftgeneric_new(const char *path)
{
    TagLib_File *f;
    struct tfile *ret;

    assert_not_null(path);

    f = taglib_file_new(path);
    if (f == NULL || !taglib_file_is_valid(f)) {
        warnx("taglib: %s is not a valid music file", path);
        return (NULL);
    }

    ret = xcalloc(1, sizeof(struct tfile));
    ret->_private.data = f;
    ret->path          = path;
    TFILE_MEMBER_F(ftgeneric,ret);

    return (ret);
}


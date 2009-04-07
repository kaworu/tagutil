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


#define T_TAGLIB_GETTER_HOOK(X)                           \
__CONCAT(ftgeneric_,X)(const struct tfile *self)          \
{                                                         \
    TagLib_File *f;                                       \
    assert_not_null(self);                                \
    assert_not_null(self->_data);                         \
    f = self->_data;                                      \
    return (__CONCAT(taglib_tag_,X)(taglib_file_tag(f))); \
}
__t__nonnull(1)
const char * T_TAGLIB_GETTER_HOOK(artist)
__t__nonnull(1)
const char * T_TAGLIB_GETTER_HOOK(album)
__t__nonnull(1)
const char * T_TAGLIB_GETTER_HOOK(comment)
__t__nonnull(1)
const char * T_TAGLIB_GETTER_HOOK(genre)
__t__nonnull(1)
const char * T_TAGLIB_GETTER_HOOK(title)
__t__nonnull(1)
unsigned int T_TAGLIB_GETTER_HOOK(track)
__t__nonnull(1)
unsigned int T_TAGLIB_GETTER_HOOK(year)


#define T_TAGLIB_SETTER_HOOK(X,TYPE)                                    \
int                                                                     \
__CONCAT(ftgeneric_set_,X)(struct tfile *self, TYPE newval)             \
{                                                                       \
    TagLib_File *f;                                                     \
    assert_not_null(self);                                              \
    assert_not_null(self->_data);                                       \
    f = self->_data;                                                    \
    __CONCAT(taglib_tag_set_,X)(taglib_file_tag(f), newval);            \
    return (0);                                                         \
}
__t__nonnull(1) __t__nonnull(2)
T_TAGLIB_SETTER_HOOK(artist, const char *)
__t__nonnull(1) __t__nonnull(2)
T_TAGLIB_SETTER_HOOK(album, const char *)
__t__nonnull(1) __t__nonnull(2)
T_TAGLIB_SETTER_HOOK(comment, const char *)
__t__nonnull(1) __t__nonnull(2)
T_TAGLIB_SETTER_HOOK(genre, const char *)
__t__nonnull(1) __t__nonnull(2)
T_TAGLIB_SETTER_HOOK(title, const char *)
__t__nonnull(1)
T_TAGLIB_SETTER_HOOK(track, unsigned int)
__t__nonnull(1)
T_TAGLIB_SETTER_HOOK(year, unsigned int)


int
ftgeneric_destroy(struct tfile *self)
{
    TagLib_File *f;

    assert_not_null(self);
    assert_not_null(self->_data);

    f = self->_data;
    taglib_tag_free_strings();
    taglib_file_free(f);

    free(self);
    return (0);
}


int
ftgeneric_save(struct tfile *self)
{
    TagLib_File *f;

    assert_not_null(self);
    assert_not_null(self->_data);

    f = self->_data;

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

    /* taglib specific init */
    taglib_set_strings_unicode(has_match(getenv("LC_ALL"), "utf-?8"));
    taglib_set_string_management_enabled(true);

    f = taglib_file_new(path);
    if (f == NULL || !taglib_file_is_valid(f)) {
        warnx("taglib: %s is not a valid music file", path);
        return (NULL);
    }

    ret = xcalloc(1, sizeof(struct tfile));
    ret->_data = f;
    strlcpy(ret->path, path, sizeof(ret->path));
    TFILE_MEMBER_F(ftgeneric,ret);

    return (ret);
}


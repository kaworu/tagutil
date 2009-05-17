/*
 * t_ftflac.c
 *
 * FLAC format handler using libFLAC.
 */
#include "t_config.h"

#include <metadata.h>

#include "t_file.h"
#include "t_ftflac.h"
#include "t_toolkit.h"


#define T_TAGLIB_GETTER_HOOK(X)                           \
__CONCAT(ftflac_,X)(const struct tfile *self)          \
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
__CONCAT(ftflac_set_,X)(struct tfile *self, TYPE newval)             \
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
ftflac_destroy(struct tfile *self)
{
    FLAC__Metadata_Chain *chain;

    assert_not_null(self);
    assert_not_null(self->_data);

    chain = self->_data;
    FLAC__metadata_chain_delete(chain);

    xfree(self);
    return (0);
}


int
ftflac_save(struct tfile *self)
{
    FLAC__Metadata_Chain *chain;

    assert_not_null(self);
    assert_not_null(self->_data);

    chain = self->_data;
    FLAC__metadata_sort_padding(chain);
    if (FLAC__metadata_chain_write(chain, true, false))
        return (0);
    else
        return (1);
}


struct tfile *
ftflac_new(const char *path)
{
    FLAC__Metadata_Chain *chain;
    struct tfile *ret;

    assert_not_null(path);

    chain = FLAC__metadata_chain_new();
    if (chain == NULL)
        warnx("libflac: '%s' is not a valid music file", path);
        return (NULL);
    }

    ret = xcalloc(1, sizeof(struct tfile));
    ret->_data = chain;
    strlcpy(ret->path, path, sizeof(ret->path));
    TFILE_MEMBER_F(ftflac,ret);

    return (ret);
}


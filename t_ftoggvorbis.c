/*
 * t_ftoggvorbis.c
 *
 * Ogg/Vorbis backend, using libvorbis and libvorbisfile
 */
#include <stdbool.h>
#include <string.h>

/* Vorbis headers */
#include <vorbis/vorbisfile.h>
#include <vorbis/codec.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_file.h"
#include "t_ftoggvorbis.h"


struct ftoggvorbis_data {
    struct OggVorbis_File *vf;
    struct vorbis_comment *vc;
};


_t__nonnull(1)
void ftoggvorbis_destroy(struct tfile *restrict self);
_t__nonnull(1)
bool ftoggvorbis_save(struct tfile *restrict self);

_t__nonnull(1) _t__nonnull(2)
char * ftoggvorbis_get(const struct tfile *restrict self,
        const char *restrict key);
_t__nonnull(1) _t__nonnull(2) _t__nonnull(3)
enum tfile_set_status ftoggvorbis_set(struct tfile *restrict self,
        const char *restrict key, const char *restrict newval);

_t__nonnull(1) _t__nonnull(2)
long ftoggvorbis_tagkeys(const struct tfile *restrict self, char ***kptr);


void
ftoggvorbis_destroy(struct tfile *restrict self)
{
    struct ftoggvorbis_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;

    ov_clear(d->vf);
    freex(d->vf);
    freex(self);
}


bool
ftoggvorbis_save(struct tfile *restrict self)
{
#if 0
    struct ftoggvorbis_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;
    FLAC__metadata_chain_sort_padding(d->chain);
    if (FLAC__metadata_chain_write(d->chain, /* padding */true,
                /* preserve_file_stats */false))
        return (true);
    else
        return (false);
#endif
    return true;
}


char *
ftoggvorbis_get(const struct tfile *restrict self, const char *restrict key)
{
    char *s, *ret;
    struct ftoggvorbis_data *d;

    assert_not_null(self);
    assert_not_null(self->data);
    assert_not_null(key);

    d = self->data;
    s = xstrdup(key);
    ret = vorbis_comment_query(d->vc, s, /* FIXME */0);
    freex(s);

    if (ret == NULL)
        return (NULL);
    else
        return (xstrdup(ret));
}


enum tfile_set_status
ftoggvorbis_set(struct tfile *restrict self, const char *restrict key,
        const char *restrict newval)
{
    char *k, *n;
    struct ftoggvorbis_data *d;

    assert_not_null(self);
    assert_not_null(self->data);
    assert_not_null(key);

    d = self->data;
    k = xstrdup(key);
    n = xstrdup(newval);
    /* FIXME: utf8encode(n) && check k */
    vorbis_comment_add_tag(d->vc, k, n);
    freex(k);
    freex(n);
    return (TFILE_SET_STATUS_LIBERROR);
}


long
ftoggvorbis_tagkeys(const struct tfile *restrict self, char ***kptr)
{
    char **ret, *tag, *eq;
    long i, count;
    size_t len;
    struct ftoggvorbis_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;
    count = (long)d->vc->comments;

    if (kptr != NULL) {
        ret = xcalloc(count, sizeof(char *));
        for (i = 0; i < count; i++) {
            tag = d->vc->user_comments[i];
            eq  = strchr(tag, '=');
            if (eq == NULL) {
                errx(-1, "invalid comment `%s' (%s backend)", self->path,
                        self->lib);
                /* NOTREACHED */
            }
            len = eq - tag + 1;
            ret[i] = xcalloc(len, sizeof(char));
            memcpy(ret[i], tag, len - 1);
            strtolower(ret[i]);
        }
        *kptr = ret;
    }

    return (count);
}


void
ftoggvorbis_init(void)
{
    return;
}


struct tfile *
ftoggvorbis_new(const char *restrict path)
{
    struct tfile *ret;
    int i;
    size_t size;
    char *s;
    struct OggVorbis_File *vf;
    struct vorbis_comment *vc;
    struct ftoggvorbis_data *d;

    assert_not_null(path);

    vf = xmalloc(sizeof(struct OggVorbis_File));
    s = xstrdup(path);
    i = ov_fopen(s, vf);
    freex(s);
    if (i != 0) {
        /* XXX: check OV_EFAULT or OV_EREAD? */
        freex(vf);
        return (NULL);
    }
    assert(vc = ov_comment(vf, -1));

    size = (strlen(path) + 1) * sizeof(char);
    ret = xmalloc(sizeof(struct tfile) + sizeof(struct ftoggvorbis_data) + size);

    d = (struct ftoggvorbis_data *)(ret + 1);
    d->vf = vf;
    d->vc = vc;
    ret->data = d;

    s = (char *)(d + 1);
    (void)strlcpy(s, path, size);
    ret->path = s;

    ret->create   = ftoggvorbis_new;
    ret->save     = ftoggvorbis_save;
    ret->destroy  = ftoggvorbis_destroy;
    ret->get      = ftoggvorbis_get;
    ret->set      = ftoggvorbis_set;
    ret->tagkeys  = ftoggvorbis_tagkeys;

    ret->lib = "libvorbis";
    return (ret);
}


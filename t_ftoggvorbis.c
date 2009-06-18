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


struct t_ftoggvorbis_data {
    struct OggVorbis_File *vf;
    struct vorbis_comment *vc;
};


_t__nonnull(1)
void t_ftoggvorbis_destroy(struct t_file *restrict self);
_t__nonnull(1)
bool t_ftoggvorbis_save(struct t_file *restrict self);

_t__nonnull(1) _t__nonnull(2)
char * t_ftoggvorbis_get(const struct t_file *restrict self,
        const char *restrict key);
_t__nonnull(1) _t__nonnull(2) _t__nonnull(3)
enum t_file_set_status t_ftoggvorbis_set(struct t_file *restrict self,
        const char *restrict key, const char *restrict newval);

_t__nonnull(1) _t__nonnull(2)
long t_ftoggvorbis_tagkeys(const struct t_file *restrict self, char ***kptr);


void
t_ftoggvorbis_destroy(struct t_file *restrict self)
{
    struct t_ftoggvorbis_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;

    ov_clear(d->vf);
    freex(d->vf);
    freex(self);
}


bool
t_ftoggvorbis_save(struct t_file *restrict self)
{
#if 0
    struct t_ftoggvorbis_data *d;

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
t_ftoggvorbis_get(const struct t_file *restrict self, const char *restrict key)
{
    char *s, *ret;
    struct t_ftoggvorbis_data *d;

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


enum t_file_set_status
t_ftoggvorbis_set(struct t_file *restrict self, const char *restrict key,
        const char *restrict newval)
{
    char *k, *n;
    struct t_ftoggvorbis_data *d;

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
t_ftoggvorbis_tagkeys(const struct t_file *restrict self, char ***kptr)
{
    char **ret, *tag, *eq;
    long i, count;
    size_t len;
    struct t_ftoggvorbis_data *d;

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
            t_strtolower(ret[i]);
        }
        *kptr = ret;
    }

    return (count);
}


void
t_ftoggvorbis_init(void)
{
    return;
}


struct t_file *
t_ftoggvorbis_new(const char *restrict path)
{
    struct t_file *ret;
    int i;
    size_t size;
    char *s;
    struct OggVorbis_File *vf;
    struct vorbis_comment *vc;
    struct t_ftoggvorbis_data *d;

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
    ret = xmalloc(sizeof(struct t_file) + sizeof(struct t_ftoggvorbis_data) + size);

    d = (struct t_ftoggvorbis_data *)(ret + 1);
    d->vf = vf;
    d->vc = vc;
    ret->data = d;

    s = (char *)(d + 1);
    (void)strlcpy(s, path, size);
    ret->path = s;

    ret->create   = t_ftoggvorbis_new;
    ret->save     = t_ftoggvorbis_save;
    ret->destroy  = t_ftoggvorbis_destroy;
    ret->get      = t_ftoggvorbis_get;
    ret->set      = t_ftoggvorbis_set;
    ret->tagkeys  = t_ftoggvorbis_tagkeys;

    ret->lib = "libvorbis";
    return (ret);
}


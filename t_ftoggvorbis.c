/*
 * t_ftoggvorbis.c
 *
 * Ogg/Vorbis backend, using libvorbis and libvorbisfile
 */
#include <stdbool.h>
#include <string.h>

/* Vorbis headers */
#include "vorbis/vorbisfile.h"
#include "vorbis/codec.h"

#include "t_config.h"
#include "t_toolkit.h"
#include "t_strbuffer.h"
#include "t_file.h"
#include "t_ftoggvorbis.h"


struct t_ftoggvorbis_data {
    struct OggVorbis_File *vf;
    struct vorbis_comment *vc;
};


_t__nonnull(1)
void t_ftoggvorbis_destroy(struct t_file *restrict file);

_t__nonnull(1)
bool t_ftoggvorbis_save(struct t_file *restrict file);

_t__nonnull(1)
struct t_taglist * t_ftoggvorbis_get(struct t_file *restrict file,
        const char *restrict key);

_t__nonnull(1)
bool t_ftoggvorbis_clear(struct t_file *restrict file,
        const struct t_taglist *T);

_t__nonnull(1) _t__nonnull(2)
bool t_ftoggvorbis_add(struct t_file *restrict file,
        const struct t_taglist *T);


void
t_ftoggvorbis_destroy(struct t_file *restrict file)
{
    struct t_ftoggvorbis_data *data;

    assert_not_null(file);
    assert_not_null(file->data);

    data = file->data;

    ov_clear(data->vf);
    freex(data->vf);
    freex(file);
}


bool
t_ftoggvorbis_save(struct t_file *restrict file)
{
    assert_not_null(file);
    t_error_clear(file);

    /* FIXME */
    t_error_set(file, "%s: read-only support", file->lib);
    return (false);
}


struct t_taglist *
t_ftoggvorbis_get(struct t_file *restrict file, const char *restrict key)
{
    int i;
    char *copy, *eq;
    const char *c;
    size_t keylen;
    struct t_taglist *T;
    struct t_ftoggvorbis_data *data;

    assert_not_null(file);
    assert_not_null(file->data);
    t_error_clear(file);

    data = file->data;
    T = t_taglist_new();
    if (key != NULL)
        keylen = strlen(key);

    for (i = 0; i < data->vc->comments; i++) {
        c = data->vc->user_comments[i];
        if (key != NULL) {
            if (strncasecmp(key, c, keylen) != 0 || c[keylen] != '=')
                continue;
        }
        copy = xstrdup(c);
        eq = strchr(copy, '=');
        if (eq == NULL) {
            t_error_set(file, "`%s' seems corrupted", file->path);
            freex(copy);
            t_taglist_destroy(T);
            return (NULL);
        }
        *eq = '\0';
        t_taglist_insert(T, copy, eq + 1);
        freex(copy);
    }

    return (T);
}


bool
t_ftoggvorbis_clear(struct t_file *restrict file, const struct t_taglist *T)
{
    int i, count;
    char *c;
    struct t_tag *t;
    struct t_ftoggvorbis_data *data;

    assert_not_null(file);
    assert_not_null(file->data);
    t_error_clear(file);

    data = file->data;

    if (T != NULL) {
        t_tagQ_foreach(t, T->tags) {
            for (i = 0; i < data->vc->comments; i++) {
                c = data->vc->user_comments[i];
                if (c != NULL) {
                    if (strncasecmp(t->key, c, t->keylen) == 0 &&
                            c[t->keylen] == '=')
                        freex(data->vc->user_comments[i]);
                }
            }
        }
        count = 0;
        for (i = 0; i < data->vc->comments; i++) {
            if (data->vc->user_comments[i] != NULL) {
                if (count != i) {
                    data->vc->user_comments[count] = data->vc->user_comments[i];
                    data->vc->comment_lengths[count] = data->vc->comment_lengths[i];
                }
                count++;
            }
        }
        data->vc->comments = count;
        data->vc->user_comments = xrealloc(data->vc->user_comments,
                (data->vc->comments + 1) * sizeof(*data->vc->user_comments));
        data->vc->comment_lengths = xrealloc(data->vc->comment_lengths,
                (data->vc->comments + 1) * sizeof(*data->vc->comment_lengths));
        /* vorbis_comment_add() set the last comment to NULL, we do the same */
        data->vc->user_comments[data->vc->comments]   = NULL;
        data->vc->comment_lengths[data->vc->comments] = 0;
    }
    else
        vorbis_comment_clear(data->vc);
    return (true);
}


bool
t_ftoggvorbis_add(struct t_file *restrict file, const struct t_taglist *T)
{
    size_t len;
    char *tageq;
    struct t_tag *t;
    struct t_ftoggvorbis_data *data;

    assert_not_null(T);
    assert_not_null(file);
    assert_not_null(file->data);
    t_error_clear(file);

    data = file->data;

    data->vc->user_comments = xrealloc(data->vc->user_comments,
            (data->vc->comments + T->count + 1) * sizeof(*data->vc->user_comments));
    data->vc->comment_lengths = xrealloc(data->vc->comment_lengths,
            (data->vc->comments + T->count + 1) * sizeof(*data->vc->comment_lengths));

    t_tagQ_foreach(t, T->tags) {
        /* FIXME: check vorbisness of t->key , utf8 t->value */
        len = xasprintf(&tageq, "%s=%s", t->key, t->value);
        data->vc->comment_lengths[data->vc->comments] = len;
        data->vc->user_comments[data->vc->comments]   = tageq;
        data->vc->comments++;
    }
    /* vorbis_comment_add() set the last comment to NULL, we do the same */
    data->vc->user_comments[data->vc->comments]   = NULL;
    data->vc->comment_lengths[data->vc->comments] = 0;
    return (true);
}


void
t_ftoggvorbis_init(void)
{
    return;
}


struct t_file *
t_ftoggvorbis_new(const char *restrict path)
{
    struct t_file *file;
    int i;
    char *s;
    struct OggVorbis_File *vf;
    struct vorbis_comment *vc;
    struct t_ftoggvorbis_data data;

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
    /* FIXME */
    assert(vc = ov_comment(vf, -1));

    data.vf = vf;
    data.vc = vc;

    t_file_new(path, "libvorbis", &data, sizeof(data));
    T_FILE_FUNC_INIT(file, oggvorbis);

    return (file);
}


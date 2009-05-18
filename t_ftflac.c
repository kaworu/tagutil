/*
 * t_ftflac.c
 *
 * FLAC format handler using libFLAC.
 */
#include "t_config.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include "/usr/local/include/FLAC/metadata.h"
#include "/usr/local/include/FLAC/format.h"

#include "t_file.h"
#include "t_ftflac.h"
#include "t_toolkit.h"

#define COPY        true
#define DO_NOT_COPY false

struct ftflac_data {
    FLAC__Metadata_Chain *chain;
    FLAC__StreamMetadata *vocomments; /* Vorbis Comments */
    uint32_t count;
    char **buffer, **keys;
};

__t__nonnull(1)
static inline void ftflac_reset_data(struct ftflac_data *restrict d);


__t__nonnull(1)
int ftflac_destroy(struct tfile *restrict self);
__t__nonnull(1)
int ftflac_save(struct tfile *restrict self);

__t__nonnull(1) __t__nonnull(2)
const char * ftflac_get(const struct tfile *restrict self,
        const char *restrict key);
__t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
int ftflac_set(struct tfile *restrict self, const char *restrict key,
        const char *restrict newval);

__t__nonnull(1)
int ftflac_tagcount(const struct tfile *restrict self);
__t__nonnull(1)
const char ** ftflac_tagkeys(const struct tfile *restrict self);


static inline void
ftflac_reset_data(struct ftflac_data *restrict d)
{
    uint32_t i;

    assert_not_null(d);

    if (d->keys) {
        for (i = 0; i < d->count; i++)
            xfree(d->keys[i]);
        xfree(d->keys);
    }
    for (i = 0; i < d->count; i++)
        free(d->buffer[i]);
    xfree(d->buffer);
    d->count = d->vocomments->data.vorbis_comment.num_comments;
    d->buffer = xcalloc(d->count, sizeof(char *));
}


int
ftflac_destroy(struct tfile *restrict self)
{
    uint32_t i;
    struct ftflac_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;

    if (d->keys) {
        for (i = 0; i < d->count; i++)
        free(d->keys[i]);
    }
    free(d->keys);
    for (i = 0; i < d->count; i++)
        free(d->buffer[i]);
    xfree(d->buffer);
    FLAC__metadata_chain_delete(d->chain);
    xfree(self);

    return (0);
}


int
ftflac_save(struct tfile *restrict self)
{
    struct ftflac_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;
    FLAC__metadata_chain_sort_padding(d->chain);
    if (FLAC__metadata_chain_write(d->chain, true, false))
        return (0);
    else
        return (1);
}


const char *
ftflac_get(const struct tfile *restrict self, const char *restrict key)
{
    bool b;
    struct ftflac_data *d;
    FLAC__StreamMetadata_VorbisComment_Entry e;
    int i;
    char *field_name, *field_value;

    assert_not_null(self);
    assert_not_null(self->data);
    assert_not_null(key);

    d = self->data;
    i = FLAC__metadata_object_vorbiscomment_find_entry_from(d->vocomments, 0, key);
    if (i == -1)
        return (NULL);

    if (d->buffer[i] == NULL) {
        e = d->vocomments->data.vorbis_comment.comments[i];
        b = FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(e, &field_name, &field_value);
        if (!b) {
            if (errno == ENOMEM)
                err(ENOMEM, "FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair");
            else
                abort();
        }
        assert(strcasecmp(field_name, key) == 0);
        xfree(field_name);

        d->buffer[i] = field_value;
    }

    return (d->buffer[i]);
}


int
ftflac_set(struct tfile *restrict self, const char *restrict key,
        const char *restrict newval)
{
    bool b;
    struct ftflac_data *d;
    FLAC__StreamMetadata_VorbisComment_Entry e;
    int i, ret = 0;

    assert_not_null(self);
    assert_not_null(self->data);
    assert_not_null(key);
    assert_not_null(newval);

    b = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&e, key, newval);
    if (!b) {
        if (errno == ENOMEM)
            err(ENOMEM, "FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair");
        else {
            warnx("invalid Vorbis tag pair: `%s' , `%s'\n", key, newval);
            return (1);
        }
    }

    d = self->data;
    i = FLAC__metadata_object_vorbiscomment_find_entry_from(d->vocomments, 0, key);
    if (i == -1) {
    /* doesn't already exist, append it */
        if (newval[0] == '\0')
            warnx("try to create a tag with empty value");
        else {
            b = FLAC__metadata_object_vorbiscomment_append_comment(d->vocomments, e, DO_NOT_COPY);
            if (!b) {
                /* FIXME: free(e) */
                ret = 3;
            }
            else
                ftflac_reset_data(d);
        }
    }
    else {
        if (newval[0] == '\0') {
        /* tag key exist, but set to empty, so delete it */
            b = FLAC__metadata_object_vorbiscomment_delete_comment(d->vocomments, i);
            if (!b)
                err(ENOMEM, "FLAC__metadata_object_vorbiscomment_delete_comment");
        }
        else {
            b = FLAC__metadata_object_vorbiscomment_replace_comment(d->vocomments, e, true, DO_NOT_COPY);
            if (!b) {
                if (errno == ENOMEM)
                    err(ENOMEM, "FLAC__metadata_object_vorbiscomment_replace_comment");
                else {
                    warnx("invalid Vorbis tag pair: `%s' , `%s'\n", key, newval);
                    return (1);
                }
            }
        }
        ftflac_reset_data(d);
    }

    return (ret);
}


int
ftflac_tagcount(const struct tfile *restrict self)
{
    struct ftflac_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;
    return ((int)d->count); /* XXX: really ok? */
}


const char **
ftflac_tagkeys(const struct tfile *restrict self)
{
    uint32_t i, j;
    size_t len;
    struct ftflac_data *d;
    FLAC__StreamMetadata_VorbisComment_Entry e;
    char *field_name, *field_value;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;

    if (d->keys) {
        for (i = 0; i < d->count; i++)
            xfree(d->keys[i]);
        xfree(d->keys);
    }

    d->keys = xcalloc(d->count, sizeof(char *));

    for (i = 0; i < d->count; i++) {
        e = d->vocomments->data.vorbis_comment.comments[i];
        if (!FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(e, &field_name, &field_value))
            errx(-1, "FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair"); /* FIXME */

        len = strlen(field_name);
        for (j = 0; j < len; j++)
            field_name[j] = tolower(field_name[j]);
        d->keys[i] = field_name;

        if (d->buffer[i] == NULL)
            d->buffer[i] = field_value;
        else
            xfree(field_value);
    }

    return (d->keys);
}


struct tfile *
ftflac_new(const char *restrict path)
{
    FLAC__Metadata_Chain *chain;
    FLAC__Metadata_Iterator *it;
    FLAC__StreamMetadata *vocomments;
    struct tfile *ret;
    char *s;
    size_t size;
    struct ftflac_data *d;

    assert_not_null(path);

    chain = FLAC__metadata_chain_new();
    if (chain == NULL)
        err(ENOMEM, "FLAC__metadata_chain_new");
    it = FLAC__metadata_iterator_new();
    if (it == NULL)
        err(ENOMEM, "FLAC__metadata_iterator_new");

    if(!FLAC__metadata_chain_read(chain, path))
        return (NULL);
    FLAC__metadata_iterator_init(it, chain);

    vocomments = NULL;
    do {
        if (FLAC__metadata_iterator_get_block_type(it) == FLAC__METADATA_TYPE_VORBIS_COMMENT)
            vocomments = FLAC__metadata_iterator_get_block(it);
    } while (vocomments == NULL && FLAC__metadata_iterator_next(it));
    if (vocomments == NULL)
        errx(-1, "couldn't find StreamMetadata VORBIS_COMMENT"); /* FIXME: create a new block VORBIS_COMMENT */
    FLAC__metadata_iterator_delete(it);

    size = (strlen(path) + 1) * sizeof(char);
    ret = xmalloc(sizeof(struct tfile) + sizeof(struct ftflac_data) + size);

    d = (struct ftflac_data *)(ret + 1);
    d->chain      = chain;
    d->vocomments = vocomments;
    d->count      = vocomments->data.vorbis_comment.num_comments;
    d->buffer     = xcalloc(d->count, sizeof(char *));
    d->keys       = NULL;
    ret->data = d;

    s = (char *)(d + 1);
    strlcpy(s, path, size);
    ret->path = s;

    ret->create   = ftflac_new;
    ret->save     = ftflac_save;
    ret->destroy  = ftflac_destroy;
    ret->get      = ftflac_get;
    ret->set      = ftflac_set;
    ret->tagcount = ftflac_tagcount;
    ret->tagkeys  = ftflac_tagkeys;

    return (ret);
}


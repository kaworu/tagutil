/*
 * t_ftflac.c
 *
 * FLAC format handler using libFLAC.
 */
#include <stdbool.h>
#include <string.h>

/* libFLAC headers */
#include <metadata.h>
#include <format.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_file.h"
#include "t_ftflac.h"


struct t_ftflac_data {
    FLAC__Metadata_Chain *chain;
    FLAC__StreamMetadata *vocomments; /* Vorbis Comments */
};


_t__nonnull(1)
void t_ftflac_destroy(struct t_file *restrict self);
_t__nonnull(1)
bool t_ftflac_save(struct t_file *restrict self);

_t__nonnull(1) _t__nonnull(2)
char * t_ftflac_get(const struct t_file *restrict self,
        const char *restrict key);
_t__nonnull(1) _t__nonnull(2) _t__nonnull(3)
enum t_file_set_status t_ftflac_set(struct t_file *restrict self,
        const char *restrict key, const char *restrict newval);

_t__nonnull(1) _t__nonnull(2)
long t_ftflac_tagkeys(const struct t_file *restrict self, char ***kptr);


void
t_ftflac_destroy(struct t_file *restrict self)
{
    struct t_ftflac_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;

    FLAC__metadata_chain_delete(d->chain);
    freex(self);
}


bool
t_ftflac_save(struct t_file *restrict self)
{
    struct t_ftflac_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;
    FLAC__metadata_chain_sort_padding(d->chain);
    if (FLAC__metadata_chain_write(d->chain, /* padding */true,
                /* preserve_file_stats */false))
        return (true);
    else
        return (false);
}


char *
t_ftflac_get(const struct t_file *restrict self, const char *restrict key)
{
    bool b;
    struct t_ftflac_data *d;
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

    e = d->vocomments->data.vorbis_comment.comments[i];
    b = FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(e, &field_name, &field_value);
    if (!b) {
        if (errno == ENOMEM)
            err(ENOMEM, "t_ftflac_get: FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair");
        else {
            warnx("t_ftflac_get: `%s' seems corrupted", self->path);
            return (NULL);
        }
    }
    assert(strcasecmp(field_name, key) == 0 && "libFLAC returned a bad key");
    freex(field_name);

    return (field_value);
}


enum t_file_set_status
t_ftflac_set(struct t_file *restrict self, const char *restrict key,
        const char *restrict newval)
{
    bool b;
    struct t_ftflac_data *d;
    FLAC__StreamMetadata_VorbisComment_Entry e;
    int i;
    char *mykey;

    assert_not_null(self);
    assert_not_null(self->data);
    assert_not_null(key);

    d = self->data;
    i = FLAC__metadata_object_vorbiscomment_find_entry_from(d->vocomments, 0, key);
    if (newval == NULL) {
    /* we are asked to delete it */
        if (i == -1)
            warnx("t_ftflac_set: try to delete a tag that is not set: `%s'", key);
        else {
            b = FLAC__metadata_object_vorbiscomment_delete_comment(d->vocomments, i);
            if (!b)
                err(ENOMEM, "t_ftflac_set: FLAC__metadata_object_vorbiscomment_delete_comment");
        }
    }
    else {
        /* create the new entry */
        mykey = xstrdup(key);
        t_strtoupper(mykey);
        b = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&e, mykey, newval);
        freex(mykey);
        if (!b) {
            if (errno == ENOMEM)
                err(ENOMEM, "t_ftflac_set: FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair");
            else {
                warnx("t_ftflac_set: invalid Vorbis tag pair: `%s', `%s'", key, newval);
                return (TFILE_SET_STATUS_BADARG);
            }
        }

        if (i == -1) {
        /* doesn't already exist, append it */
            if (t_strempty(newval))
                warnx("t_ftflac_set: create a tag with empty value: `%s'", key);
            b = FLAC__metadata_object_vorbiscomment_append_comment(d->vocomments, e, /* copy */false);
            if (!b) {
                freex(e.entry);
                warnx("t_ftflac_set: %s backend error", self->lib);
                return (TFILE_SET_STATUS_LIBERROR);
            }
        }
        else {
        /* tag key exist, replace it */
            b = FLAC__metadata_object_vorbiscomment_replace_comment(d->vocomments, e, true, /* copy */false);
            if (!b) {
                if (errno == ENOMEM)
                    err(ENOMEM, "t_ftflac_set: FLAC__metadata_object_vorbiscomment_replace_comment");
                else {
                    freex(e.entry);
                    warnx("t_ftflac_set: %s backend error", self->lib);
                    return (TFILE_SET_STATUS_LIBERROR);
                }
            }
        }
    }

    return (TFILE_SET_STATUS_OK);
}


long
t_ftflac_tagkeys(const struct t_file *restrict self, char ***kptr)
{
    char **ret;
    long i, count;
    bool b;
    struct t_ftflac_data *d;
    FLAC__StreamMetadata_VorbisComment_Entry e;
    char *field_name, *field_value;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;

    count = (long)(d->vocomments->data.vorbis_comment.num_comments);
    if (kptr != NULL) {
        *kptr = ret = xcalloc(count, sizeof(char *));

        for (i = 0; i < count; i++) {
            e = d->vocomments->data.vorbis_comment.comments[i];
            b = FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(e, &field_name, &field_value);
            if (!b) {
                /* FIXME: LEAK */
                warnx("t_ftflac_tagkeys: `%s' seems corrupted", self->path);
                return (-1);
            }
            freex(field_value);
            ret[i] = field_name;
            t_strtolower(field_name);
        }
    }

    return (count);
}


void
t_ftflac_init(void)
{
    return;
}


struct t_file *
t_ftflac_new(const char *restrict path)
{
    FLAC__Metadata_Chain *chain;
    FLAC__Metadata_Iterator *it;
    FLAC__StreamMetadata *vocomments;
    bool b;
    struct t_file *ret;
    char *s;
    size_t size;
    struct t_ftflac_data *d;

    assert_not_null(path);

    chain = FLAC__metadata_chain_new();
    if (chain == NULL)
        err(ENOMEM, "t_ftflac_new: FLAC__metadata_chain_new");
    if (!FLAC__metadata_chain_read(chain, path)) {
        FLAC__metadata_chain_delete(chain);
        return (NULL);
    }

    it = FLAC__metadata_iterator_new();
    if (it == NULL)
        err(ENOMEM, "t_ftflac_new: FLAC__metadata_iterator_new");
    FLAC__metadata_iterator_init(it, chain);

    vocomments = NULL;
    do {
        if (FLAC__metadata_iterator_get_block_type(it) == FLAC__METADATA_TYPE_VORBIS_COMMENT)
            vocomments = FLAC__metadata_iterator_get_block(it);
    } while (vocomments == NULL && FLAC__metadata_iterator_next(it));
    if (vocomments == NULL) {
    /* create a new block FLAC__METADATA_TYPE_VORBIS_COMMENT */
        vocomments = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
        if (vocomments == NULL)
            err(ENOMEM, "t_ftflac_new: FLAC__metadata_object_new");
        b = FLAC__metadata_iterator_insert_block_after(it, vocomments);
        if (!b)
            err(errno, "t_ftflac_new: FLAC__metadata_iterator_insert_block_after");
    }
    FLAC__metadata_iterator_delete(it);

    size = (strlen(path) + 1) * sizeof(char);
    ret = xmalloc(sizeof(struct t_file) + sizeof(struct t_ftflac_data) + size);

    d = (struct t_ftflac_data *)(ret + 1);
    d->chain      = chain;
    d->vocomments = vocomments;
    ret->data = d;

    s = (char *)(d + 1);
    (void)strlcpy(s, path, size);
    ret->path = s;

    ret->create   = t_ftflac_new;
    ret->save     = t_ftflac_save;
    ret->destroy  = t_ftflac_destroy;
    ret->get      = t_ftflac_get;
    ret->set      = t_ftflac_set;
    ret->tagkeys  = t_ftflac_tagkeys;

    ret->lib = "libFLAC";
    return (ret);
}


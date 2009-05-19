/*
 * t_ftflac.c
 *
 * FLAC format handler using libFLAC.
 */
#include "t_config.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include "metadata.h"
#include "format.h"

#include "t_file.h"
#include "t_ftflac.h"
#include "t_toolkit.h"

#define COPY        true
#define DO_NOT_COPY false

struct ftflac_data {
    FLAC__Metadata_Chain *chain;
    FLAC__StreamMetadata *vocomments; /* Vorbis Comments */
};


__t__nonnull(1)
void ftflac_destroy(struct tfile *restrict self);
__t__nonnull(1)
bool ftflac_save(struct tfile *restrict self);

__t__nonnull(1) __t__nonnull(2)
char * ftflac_get(const struct tfile *restrict self,
        const char *restrict key);
__t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
enum tfile_set_status ftflac_set(struct tfile *restrict self,
        const char *restrict key, const char *restrict newval);

__t__nonnull(1)
int ftflac_tagcount(const struct tfile *restrict self);
__t__nonnull(1)
char ** ftflac_tagkeys(const struct tfile *restrict self);


void
ftflac_destroy(struct tfile *restrict self)
{
    struct ftflac_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;

    FLAC__metadata_chain_delete(d->chain);
    xfree(self);
}


bool
ftflac_save(struct tfile *restrict self)
{
    struct ftflac_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;
    FLAC__metadata_chain_sort_padding(d->chain);
    if (FLAC__metadata_chain_write(d->chain, true, false))
        return (true);
    else
        return (false);
}


char *
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

    return (field_value);
}


enum tfile_set_status
ftflac_set(struct tfile *restrict self, const char *restrict key,
        const char *restrict newval)
{
    bool b;
    struct ftflac_data *d;
    FLAC__StreamMetadata_VorbisComment_Entry e;
    int i;

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
            return (TFILE_SET_STATUS_BADARG);
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
                warnx("%s backend error", self->lib);
                return (TFILE_SET_STATUS_LIBERROR);
            }
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
                    warnx("%s backend error", self->lib);
                    return (TFILE_SET_STATUS_LIBERROR);
                }
            }
        }
    }

    return (TFILE_SET_STATUS_OK);
}


int
ftflac_tagcount(const struct tfile *restrict self)
{
    int count;
    struct ftflac_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;
    count = (int)(d->vocomments->data.vorbis_comment.num_comments); /* XXX: cast really ok? */
    return (count);
}


char **
ftflac_tagkeys(const struct tfile *restrict self)
{
    char **ret;
    int i, count;
    struct ftflac_data *d;
    FLAC__StreamMetadata_VorbisComment_Entry e;
    char *field_name, *field_value;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;

    count = self->tagcount(self);
    ret = xcalloc(count, sizeof(char *));

    for (i = 0; i < count; i++) {
        e = d->vocomments->data.vorbis_comment.comments[i];
        if (!FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(e, &field_name, &field_value))
            errx(-1, "FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair"); /* FIXME */

        xfree(field_value);
        ret[i] = field_name;
    }

    return (ret);
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

    ret->lib = "libFLAC";
    return (ret);
}


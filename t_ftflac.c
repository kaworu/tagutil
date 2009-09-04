/*
 * t_ftflac.c
 *
 * FLAC format handler using libFLAC.
 */
#include <stdbool.h>
#include <string.h>

/* libFLAC headers */
#include "metadata.h"
#include "format.h"

#include "t_config.h"
#include "t_toolkit.h"
#include "t_file.h"
#include "t_ftflac.h"


static const char libname[] = "libFLAC";


struct t_ftflac_data {
	FLAC__Metadata_Chain	*chain;
	FLAC__StreamMetadata	*vocomments; /* Vorbis Comments */
};


_t__nonnull(1)
static void	t_ftflac_destroy(struct t_file *restrict file);

_t__nonnull(1)
static bool	t_ftflac_save(struct t_file *restrict file);

_t__nonnull(1)
static struct t_taglist * t_ftflac_get(struct t_file *restrict file,
    const char *restrict key);

_t__nonnull(1)
static bool	t_ftflac_clear(struct t_file *restrict file,
    const struct t_taglist *restrict T);

_t__nonnull(1) _t__nonnull(2)
static bool	t_ftflac_add(struct t_file *restrict file,
     const struct t_taglist *restrict T);


struct t_file *
t_ftflac_new(const char *restrict path)
{
	bool	b;
	FLAC__Metadata_Iterator	*it;
	struct t_file		*file;
	struct t_ftflac_data	 data;

	assert_not_null(path);

	data.chain = FLAC__metadata_chain_new();
	if (data.chain == NULL)
		err(ENOMEM, "t_ftflac_new: FLAC__metadata_chain_new");
	if (!FLAC__metadata_chain_read(data.chain, path)) {
		FLAC__metadata_chain_delete(data.chain);
		return (NULL);
	}

	it = FLAC__metadata_iterator_new();
	if (it == NULL)
		err(ENOMEM, "t_ftflac_new: FLAC__metadata_iterator_new");
	FLAC__metadata_iterator_init(it, data.chain);

	data.vocomments = NULL;
	do {
		if (FLAC__metadata_iterator_get_block_type(it) == FLAC__METADATA_TYPE_VORBIS_COMMENT)
			data.vocomments = FLAC__metadata_iterator_get_block(it);
	} while (data.vocomments == NULL && FLAC__metadata_iterator_next(it));
	if (data.vocomments == NULL) {
		/* create a new block FLAC__METADATA_TYPE_VORBIS_COMMENT */
		data.vocomments = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
		if (data.vocomments == NULL)
			err(ENOMEM, "t_ftflac_new: FLAC__metadata_object_new");
		b = FLAC__metadata_iterator_insert_block_after(it, data.vocomments);
		if (!b)
			err(errno, "t_ftflac_new: FLAC__metadata_iterator_insert_block_after");
	}
	FLAC__metadata_iterator_delete(it);

	file = t_file_new(path, libname, &data, sizeof(data));
	T_FILE_FUNC_INIT(file, flac);

	return (file);
}


static void
t_ftflac_destroy(struct t_file *restrict file)
{
	struct t_ftflac_data *data;

	assert_not_null(file);
	assert_not_null(file->data);

	data = file->data;

	FLAC__metadata_chain_delete(data->chain);
	t_error_clear(file);
	freex(file);
}


static bool
t_ftflac_save(struct t_file *restrict file)
{
	bool	ok;
	struct t_ftflac_data	*data;

	assert_not_null(file);
	assert_not_null(file->data);
	t_error_clear(file);

	data = file->data;

	FLAC__metadata_chain_sort_padding(data->chain);
	ok = FLAC__metadata_chain_write(data->chain,
	    /* padding */true,
	    /* preserve_file_stats */false);
	if (!ok) {
		FLAC__Metadata_ChainStatus status;
		status = FLAC__metadata_chain_status(data->chain);
		t_error_set(file, "%s", FLAC__Metadata_ChainStatusString[status]);
	}

	return (ok);
}


static struct t_taglist *
t_ftflac_get(struct t_file *restrict file, const char *restrict key)
{
	bool	 b;
	char	*field_name  = NULL;
	char	*field_value = NULL;
	int	 i = 0;
	uint32_t count;
	struct t_taglist	*T;
	struct t_ftflac_data	*data;
	FLAC__StreamMetadata_VorbisComment_Entry e;

	assert_not_null(file);
	assert_not_null(file->data);
	t_error_clear(file);

	data = file->data;
	T = t_taglist_new();
	count = data->vocomments->data.vorbis_comment.num_comments;

	for (;;) {
		if (key != NULL)
			i = FLAC__metadata_object_vorbiscomment_find_entry_from(data->vocomments, i, key);
		if (i == -1 || (uint32_t)i == count)
			break;

		e = data->vocomments->data.vorbis_comment.comments[i++];
		b = FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(e, &field_name, &field_value);
		if (!b) {
			if (errno) {
				t_error_set(file, "FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair: %s",
				    strerror(errno));
			} else
				t_error_set(file, "`%s' seems corrupted", file->path);
			t_taglist_destroy(T);
			freex(field_name);
			freex(field_value);
			return (NULL);
		}
		if (key != NULL)
			assert(strcasecmp(field_name, key) == 0);
		t_taglist_insert(T, t_strtolower(field_name), field_value);
		freex(field_name);
		freex(field_value);
	}

	return (T);
}


static bool
t_ftflac_clear(struct t_file *restrict file, const struct t_taglist *restrict T)
{
	bool	b;
	int	i = 0;
	struct t_ftflac_data	*data;
	struct t_tag		*t;
	struct t_tag		*last;

	assert_not_null(file);
	assert_not_null(file->data);
	t_error_clear(file);

	data = file->data;
	if (T != NULL) {
		if (T->count == 0)
			return (true);
		t = t_tagQ_first(T->tags);
		last = t_tagQ_last(T->tags);
	}
	for (;;) {
		if (T != NULL) {
			i = FLAC__metadata_object_vorbiscomment_find_entry_from(data->vocomments, 0, t->key);
			if (i == -1) {
				if (t == last)
					break;
				else {
					t = t_tagQ_next(t);
					i = 0;
					continue;
				}
			}
		} else if (data->vocomments->data.vorbis_comment.num_comments == 0)
			break;
		b = FLAC__metadata_object_vorbiscomment_delete_comment(data->vocomments, i);
		if (!b) {
			t_error_set(file, "FLAC__metadata_object_vorbiscomment_delete_comment: %s",
			    strerror(ENOMEM));
			return (false);
		}
	}

	return (true);
}


static bool
t_ftflac_add(struct t_file *restrict file, const struct t_taglist *restrict T)
{
	bool	b;
	struct t_ftflac_data *data;
	struct t_tag *t;
	FLAC__StreamMetadata_VorbisComment_Entry e;

	assert_not_null(file);
	assert_not_null(file->data);
	assert_not_null(T);
	t_error_clear(file);

	data = file->data;
	t_tagQ_foreach(t, T->tags) {
		b = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&e, t->key, t->value);
		if (!b) {
			if (errno) {
				t_error_set(file, "FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair: %s",
				    strerror(errno));
			} else
				t_error_set(file, "invalid Vorbis tag pair: `%s', `%s'", t->key, t->value);
			return (false);
		}
		b = FLAC__metadata_object_vorbiscomment_append_comment(data->vocomments, e, /* copy */false);
		if (!b) {
			freex(e.entry);
			if (errno) {
				t_error_set(file, "FLAC__metadata_object_vorbiscomment_append_comment: %s",
				    strerror(errno));
			} else
				t_error_set(file, "invalid Vorbis tag entry created with: `%s', `%s'", t->key, t->value);
			return (false);
		}
	}

	return (true);
}

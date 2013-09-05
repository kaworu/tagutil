/*
 * t_ftflac.c
 *
 * FLAC format handler using libFLAC.
 */
#include <stdbool.h>
#include <string.h>

/* libFLAC headers */
#include "FLAC/metadata.h"
#include "FLAC/format.h"

#include "t_config.h"
#include "t_tune.h"
#include "t_backend.h"


static const char libid[] = "t_tune_libFLAC";


struct t_flac_data {
	FLAC__Metadata_Chain	*chain;
	FLAC__StreamMetadata	*vocomments; /* Vorbis Comments */
};


static int		 t_ftflac_init(struct t_tune *tune);
static struct t_taglist	*t_ftflac_read(struct t_tune *tune);
static int		 t_ftflac_write(struct t_tune *tune, const struct t_taglist *tlist);
static void		 t_ftflac_clear(struct t_tune *tune);


static int
t_ftflac_init(struct t_tune *tune)
{
	FLAC__Metadata_Iterator *it;
	struct t_flac_data *data;

	assert_not_null(tune);
	assert_not_null(tune->path);

	data = calloc(1, sizeof(struct t_flac_data));
	if (data == NULL)
		goto error0;

	data->chain = FLAC__metadata_chain_new();
	if (data->chain == NULL)
		goto error0;
	if (!FLAC__metadata_chain_read(data->chain, tune->path))
		goto error1;

	it = FLAC__metadata_iterator_new();
	if (it == NULL)
		goto error1;
	FLAC__metadata_iterator_init(it, data->chain);

	data->vocomments = NULL;
	do {
		if (FLAC__metadata_iterator_get_block_type(it) == FLAC__METADATA_TYPE_VORBIS_COMMENT)
			data->vocomments = FLAC__metadata_iterator_get_block(it);
	} while (data->vocomments == NULL && FLAC__metadata_iterator_next(it));
	if (data->vocomments == NULL) {
		/* create a new block FLAC__METADATA_TYPE_VORBIS_COMMENT */
		data->vocomments = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
		if (data->vocomments == NULL)
			goto error2;
		if (!FLAC__metadata_iterator_insert_block_after(it, data->vocomments))
			goto error3;
	}
	FLAC__metadata_iterator_delete(it);

	tune->backend = t_tune_flac_backend();
	tune->opaque  = data;
	return (0);
	/* NOTREACHED */

error3:
	FLAC__metadata_object_delete(data->vocomments);
error2:
	FLAC__metadata_iterator_delete(it);
error1:
	FLAC__metadata_chain_delete(data->chain);
error0:
	free(data);
	return (-1);
}

static struct t_taglist *
t_ftflac_read(struct t_tune *tune)
{
	char *field_name  = NULL;
	char *field_value = NULL;
	struct t_taglist   *tlist = NULL;
	struct t_flac_data *data;
	FLAC__StreamMetadata_VorbisComment_Entry e;
	uint32_t i;

	assert_not_null(tune);
	assert_not_null(tune->opaque);
	assert(tune->backend->libid == libid);

	data = tune->opaque;
	if ((tlist = t_taglist_new()) == NULL)
		return (NULL);

	for (i = 0; i < data->vocomments->data.vorbis_comment.num_comments; i++) {
		e = data->vocomments->data.vorbis_comment.comments[i];
		if (!FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(e,
		    &field_name, &field_value))
			goto error;
		if ((t_taglist_insert(tlist, t_strtolower(field_name), field_value)) == -1)
			goto error;
		free(field_name);
		free(field_value);
		field_name  = NULL;
		field_value = NULL;
	}

	return (tlist);
	/* NOTREACHED */
error:
	t_taglist_delete(tlist);
	free(field_name);
	free(field_value);
	return (NULL);
}

static int
t_ftflac_write(struct t_tune *tune, const struct t_taglist *tlist)
{
	struct t_flac_data *data;
	struct t_tag *t;
	FLAC__StreamMetadata_VorbisComment_Entry e;

	assert_not_null(tune);
	assert_not_null(tune->opaque);
	assert(tune->backend->libid == libid);

	data = tune->opaque;

	/* clear all the tags */
	while (data->vocomments->data.vorbis_comment.num_comments == 0) {
		if (!FLAC__metadata_object_vorbiscomment_delete_comment(data->vocomments, 0))
			return (-1);

	/* load the tlist */
	TAILQ_FOREACH(t, tlist->tags, entries) {
		if (!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&e, t->key, t->val))
			return (-1);
		if(!FLAC__metadata_object_vorbiscomment_append_comment(data->vocomments, e, /* copy */false)) {
			free(e.entry);
			return (-1);
		}
	}

	/* do the write */
	FLAC__metadata_chain_sort_padding(data->chain);
	if (!FLAC__metadata_chain_write(data->chain, /* padding */true, /* preserve_file_stats */false))
		return (-1);

	return (0);
}

static void
t_ftflac_clear(struct t_tune *tune)
{
	struct t_flac_data *data;

	assert_not_null(tune);
	assert_not_null(tune->opaque);
	assert(tune->backend->libid == libid);

	data = tune->opaque;
	FLAC__metadata_object_delete(data->vocomments);
	FLAC__metadata_chain_delete(data->chain);
}


#include "t_file.h"
static struct t_file	*t_file_new(const char *path) t__deprecated;
static void		 t_file_destroy(struct t_file *file) t__deprecated;
static bool		 t_file_save(struct t_file *file) t__deprecated;
static struct t_taglist	*t_file_get(struct t_file *file, const char *key) t__deprecated;
static bool		 t_file_clear(struct t_file *file, const struct t_taglist *T) t__deprecated;
static bool		 t_file_add(struct t_file *file, const struct t_taglist *T) t__deprecated;


static struct t_file *
t_tune_wrapper_file_new(const char *path)
{
	struct t_tune tune;

	assert_not_null(path);

	if (t_tune_init(&tune, path) == -1)
		return (NULL);
	T_FILE_NEW(file, path, tune);
	file->libid = tune.backend->libid;

	return (file);
}



struct t_backend *
t_tune_flac_backend(void)
{
	static struct t_backend b = {
		.libid		= libid,
		.desc		=
		    "Wrapper to t_tune flac files format, use `Vorbis comment' metadata tags.",
		.init		= t_ftflac_init,
		.read		= t_ftflac_read,
		.write		= t_ftflac_write,
		.clear		= t_ftflac_clear,
	};
	b.ctor = t_tune_wrapper_file_new;
	return (&b);
}



static void
t_file_destroy(struct t_file *file)
{
	struct t_tune *tune;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);

	tune = file->data;

	t_tune_clear(tune);
	T_FILE_DESTROY(file);
}


static bool
t_file_save(struct t_file *file)
{
	struct t_tune *tune;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);

	tune = file->data;

	return (t_tune_save(tune) == 0);
}


static struct t_taglist *
t_file_get(struct t_file *file, const char *key)
{ }


static bool
t_file_clear(struct t_file *file, const struct t_taglist *T)
{ }


static bool
t_file_add(struct t_file *file, const struct t_taglist *T)
{ }

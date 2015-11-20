/*
 * t_ftflac.c
 *
 * FLAC format handler using libFLAC.
 *
 * XXX: error handling could be better since libFLAC has a very good API. For
 * now it's mostly silent.
 */

/* libFLAC headers */
#include "FLAC/metadata.h"
#include "FLAC/format.h"

#include "t_config.h"
#include "t_backend.h"


static const char libid[] = "libFLAC";


struct t_ftflac_data {
	const char		*libid; /* pointer to libid */
	FLAC__Metadata_Chain	*chain;
	FLAC__StreamMetadata	*vocomments; /* Vorbis Comments */
};


struct t_backend	*t_ftflac_backend(void);

static void 		*t_ftflac_init(const char *path);
static struct t_taglist	*t_ftflac_read(void *opaque);
static int		 t_ftflac_write(void *opaque, const struct t_taglist *tlist);
static void		 t_ftflac_clear(void *opaque);

struct t_backend *
t_ftflac_backend(void)
{
	static struct t_backend b = {
		.libid		= libid,
		.desc		=
		    "Free Lossless Audio Codec (FLAC) files format",
		.init		= t_ftflac_init,
		.read		= t_ftflac_read,
		.write		= t_ftflac_write,
		.clear		= t_ftflac_clear,
	};

	return (&b);
}


static void *
t_ftflac_init(const char *path)
{
	FLAC__Metadata_Iterator *it;
	struct t_ftflac_data *data;

	assert(path != NULL);

	data = calloc(1, sizeof(struct t_ftflac_data));
	if (data == NULL)
		goto error0;
	data->libid = libid;

	data->chain = FLAC__metadata_chain_new();
	if (data->chain == NULL)
		goto error0;
	if (!FLAC__metadata_chain_read(data->chain, path))
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

	return (data);
	/* NOTREACHED */

error3:
	FLAC__metadata_object_delete(data->vocomments);
error2:
	FLAC__metadata_iterator_delete(it);
error1:
	FLAC__metadata_chain_delete(data->chain);
error0:
	free(data);
	return (NULL);
}


static struct t_taglist *
t_ftflac_read(void *opaque)
{
	char *field_name  = NULL;
	char *field_value = NULL;
	struct t_taglist   *tlist = NULL;
	struct t_ftflac_data *data;
	FLAC__StreamMetadata_VorbisComment_Entry e;
	uint32_t i;

	assert(opaque != NULL);
	data = opaque;
	assert(data->libid == libid);

	if ((tlist = t_taglist_new()) == NULL)
		return (NULL);

	for (i = 0; i < data->vocomments->data.vorbis_comment.num_comments; i++) {
		e = data->vocomments->data.vorbis_comment.comments[i];
		if (!FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(e,
		    &field_name, &field_value))
			goto error;
		if ((t_taglist_insert(tlist, field_name, field_value)) == -1)
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
t_ftflac_write(void *opaque, const struct t_taglist *tlist)
{
	struct t_ftflac_data *data;
	struct t_tag *t;
	FLAC__StreamMetadata_VorbisComment_Entry e;

	assert(opaque != NULL);
	data = opaque;
	assert(data->libid == libid);

	/* clear all the tags */
	while (data->vocomments->data.vorbis_comment.num_comments > 0) {
		if (!FLAC__metadata_object_vorbiscomment_delete_comment(data->vocomments, 0))
			return (-1);
	}

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
t_ftflac_clear(void *opaque)
{
	struct t_ftflac_data *data;

	assert(opaque != NULL);
	data = opaque;
	assert(data->libid == libid);

	FLAC__metadata_chain_delete(data->chain);
	free(data);
}

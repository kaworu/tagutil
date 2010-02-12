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
#include "t_backend.h"


static const char libid[] = "libvorbis";


struct t_oggvorbis_data {
	struct OggVorbis_File	*vf;
	struct vorbis_comment	*vc;
};


_t__nonnull(1)
static void	t_file_destroy(struct t_file *restrict file);

static struct t_file *	t_file_new(const char *restrict path);

_t__nonnull(1)
static bool	t_file_save(struct t_file *restrict file);

_t__nonnull(1)
static struct t_taglist * t_file_get(struct t_file *restrict file,
    const char *restrict key);

_t__nonnull(1)
static bool	t_file_clear(struct t_file *restrict file,
    const struct t_taglist *T);

_t__nonnull(1) _t__nonnull(2)
static bool	t_file_add(struct t_file *restrict file,
    const struct t_taglist *T);


struct t_backend *
t_oggvorbis_backend(void)
{
	static struct t_backend b = {
		.libid		= libid,
    		.desc		= "Ogg/Vorbis files format, "
		    "use `Vorbis comment' metadata tags.",
		.ctor		= t_file_new,
	};
	return (&b);
}


static struct t_file *
t_file_new(const char *restrict path)
{
	char	*s;
	int	 i;
	struct t_file *file;
	struct OggVorbis_File	*vf;
	struct vorbis_comment	*vc;
	struct t_oggvorbis_data data;

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

	T_FILE_NEW(file, path, data);
	return (file);
}


static void
t_file_destroy(struct t_file *restrict file)
{
	struct t_oggvorbis_data *data;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);

	data = file->data;

	ov_clear(data->vf);
	freex(data->vf);
	freex(file);
}


static bool
t_file_save(struct t_file *restrict file)
{
	assert_not_null(file);
	assert(file->libid == libid);
	t_error_clear(file);

#if 0
	if (!file->dirty)
		return (true);
#endif
	/* FIXME */
	t_error_set(file, "%s: read-only support", file->libid);
	return (false);
}


static struct t_taglist *
t_file_get(struct t_file *restrict file, const char *restrict key)
{
	char	*copy;
	char	*eq;
	int	 i;
	size_t	keylen;
	const char *c;
	struct t_taglist *T;
	struct t_oggvorbis_data *data;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);
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


static bool
t_file_clear(struct t_file *restrict file, const struct t_taglist *T)
{
	int	 i;
	int	 count;
	char	*c;
	struct t_tag *t;
	struct t_oggvorbis_data *data;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);
	t_error_clear(file);

	data = file->data;

	if (T != NULL) {
		TAILQ_FOREACH(t, T->tags, entries) {
			for (i = 0; i < data->vc->comments; i++) {
				c = data->vc->user_comments[i];
				if (c != NULL) {
					if (strncasecmp(t->key, c, t->keylen) == 0 &&
					    c[t->keylen] == '=') {
						freex(data->vc->user_comments[i]);
					    	file->dirty++;
					}
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
		data->vc->user_comments =
		    xrealloc(data->vc->user_comments, (data->vc->comments + 1) * sizeof(*data->vc->user_comments));
		data->vc->comment_lengths =
		    xrealloc(data->vc->comment_lengths, (data->vc->comments + 1) * sizeof(*data->vc->comment_lengths));
		/* vorbis_comment_add() set the last comment to NULL, we do the same */
		data->vc->user_comments[data->vc->comments]   = NULL;
		data->vc->comment_lengths[data->vc->comments] = 0;
	} else
		vorbis_comment_clear(data->vc);
	return (true);
}


static bool
t_file_add(struct t_file *restrict file, const struct t_taglist *T)
{
	size_t	 len;
	char	*tageq;
	struct t_tag *t;
	struct t_oggvorbis_data *data;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);
	assert_not_null(T);
	t_error_clear(file);

	data = file->data;

	data->vc->user_comments =
	    xrealloc(data->vc->user_comments, (data->vc->comments + T->count + 1) * sizeof(*data->vc->user_comments));
	data->vc->comment_lengths =
	    xrealloc(data->vc->comment_lengths, (data->vc->comments + T->count + 1) * sizeof(*data->vc->comment_lengths));

	TAILQ_FOREACH(t, T->tags, entries) {
		/* FIXME: check vorbisness of t->key , utf8 t->value */
		len = xasprintf(&tageq, "%s=%s", t->key, t->value);
		data->vc->comment_lengths[data->vc->comments] = len;
		data->vc->user_comments[data->vc->comments]   = tageq;
		data->vc->comments++;
		file->dirty++;
	}
	/* vorbis_comment_add() set the last comment to NULL, we do the same */
	data->vc->user_comments[data->vc->comments]   = NULL;
	data->vc->comment_lengths[data->vc->comments] = 0;
	return (true);
}

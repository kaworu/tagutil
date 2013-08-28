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
#include "t_file.h"
#include "t_backend.h"


static const char libid[] = "libvorbis";


struct t_oggvorbis_data {
	struct OggVorbis_File	*vf;
	struct vorbis_comment	*vc;
};


_t__nonnull(1)
static void	t_file_destroy(struct t_file *file);

static struct t_file *	t_file_new(const char *path);

_t__nonnull(1)
static bool	t_file_save(struct t_file *file);

_t__nonnull(1)
static struct t_taglist * t_file_get(struct t_file *file,
    const char *key);

_t__nonnull(1)
static bool	t_file_clear(struct t_file *file,
    const struct t_taglist *T);

_t__nonnull(1) _t__nonnull(2)
static bool	t_file_add(struct t_file *file,
    const struct t_taglist *T);

static void	fucking_save_it(const char *ipath, struct vorbis_comment *vc, const char *opath);

void
fucking_save_it(const char *ipath, struct vorbis_comment *vc, const char *opath)
{
	FILE *ofp, *ifp;
	char *buf;
	ogg_stream_state ios, oos;
	ogg_sync_state oy;
	ogg_page iog, oog;
	ogg_packet op, my_vc_packet, *target;
	vorbis_comment unused_ovc;
	vorbis_info vi;
	int npacket = 0, done = 0;
	ogg_int64_t granpos = 0;
	int prevW = 0, needflush = 0, needout = 1;

	/* create the packet holding our vorbis_comment */
	assert(vorbis_commentheader_out(vc, &my_vc_packet) == 0); /* 0 on success, OV_EIMPL on error */

	/* open files & stuff */
	vorbis_info_init(&vi);
	(void)ogg_sync_init(&oy); /* always return 0 */
	assert(ifp = fopen(ipath, "r"));
	assert(ofp = fopen(opath, "w"));

	/* main loop: read the input file into buf in order to sync pages out */
	while (!done) {
		size_t s;
		switch (ogg_sync_pageout(&oy, &iog)) {
		case 0:  /* more data needed or an internal error occurred. */
		case -1: /* stream has not yet captured sync (bytes were skipped). */
			if (feof(ifp))
				done = 1;
			else {
				assert(buf = ogg_sync_buffer(&oy, BUFSIZ));
				assert((s = fread(buf, sizeof(char), BUFSIZ, ifp)) != -1);
				assert(ogg_sync_wrote(&oy, s) != -1); /* 0 on success, -1 on error (overflow) */
			}
			break;
		case 1: /* a page was synced and returned. */
			if (npacket == 0) {
				/* init both input and output streams with the serialno of the first page */
				assert(ogg_stream_init(&ios, ogg_page_serialno(&iog)) != -1); /* 0 on success, -1 on error */
				assert(ogg_stream_init(&oos, ogg_page_serialno(&iog)) != -1); /* 0 on success, -1 on error */
			}
			/* page in the page into the input stream, in order to fetch its packets */
			assert(ogg_stream_pagein(&ios, &iog) != -1); /* 0 on success, -1 on error */
			while (ogg_stream_packetout(&ios, &op) == 1) {

				/* the second packet is the commentheader packet, we replace it with my_vc_packet */
				target = (++npacket == 2 ? &my_vc_packet : &op);

				/* insert the target packet into the output stream */
				if (npacket <= 3) {
					/* the three first packet are used to fill the vorbis info (used to compute granule) */
					vorbis_synthesis_headerin(&vi, &unused_ovc, &op);
					needflush = 1; /* flush is needed after the three first packet */
				} else {
					/* granule computation (don't ask) */
					int bs = vorbis_packet_blocksize(&vi, &op);
					s = (bs + prevW) / 4;
					if (prevW == 0)
						s = 0;
					prevW = bs;
					granpos += s;

					/* write a page if needed */
					while ((needflush && ogg_stream_flush(&oos, &oog)) ||
					    (needout && ogg_stream_pageout(&oos, &oog))) {
						assert(fwrite(oog.header, sizeof(unsigned char), oog.header_len, ofp) == oog.header_len);
						assert(fwrite(oog.body,   sizeof(unsigned char), oog.body_len,   ofp) == oog.body_len);
					}

					/* flush / out / granpos hack, idk the logic i just saw it in vcedit.c */
					needflush = needout = 0;
					if (op.granulepos == -1) {
						op.granulepos = granpos;
					} else if (granpos > op.granulepos) {
						granpos = op.granulepos;
						needflush = 1;
					} else
						needout = 1;
				}
				assert(ogg_stream_packetin(&oos, target) == 0); /* 0 on success, -1 on error */
			}
		}
	}
	oos.e_o_s = 1;
	/* forces remaining packets into a page and write it in the output file */
	if (ogg_stream_flush(&oos, &oog)) {
		assert(fwrite(oog.header, sizeof(unsigned char), oog.header_len, ofp) == oog.header_len);
		assert(fwrite(oog.body,   sizeof(unsigned char), oog.body_len,   ofp) == oog.body_len);
	}
	assert(fclose(ofp) == 0);
	(void)fclose(ifp);
	ogg_packet_clear(&my_vc_packet);
	vorbis_comment_clear(&unused_ovc);
	vorbis_info_clear(&vi);
	ogg_sync_clear(&oy);
	ogg_stream_clear(&ios);
	ogg_stream_clear(&oos);
}


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
t_file_new(const char *path)
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
	free(s);
	if (i != 0) {
		/* XXX: check OV_EFAULT or OV_EREAD? */
		free(vf);
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
t_file_destroy(struct t_file *file)
{
	struct t_oggvorbis_data *data;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);

	data = file->data;

	ov_clear(data->vf);
	free(data->vf);
	T_FILE_DESTROY(file);
}


static bool
t_file_save(struct t_file *file)
{
	assert_not_null(file);
	assert(file->libid == libid);
	t_error_clear(file);

	/* FIXME */
	t_error_set(file, "%s: read-only support", file->libid);
	return (false);

	if (!file->dirty)
		return (true);
}


static struct t_taglist *
t_file_get(struct t_file *file, const char *key)
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
			free(copy);
			t_taglist_destroy(T);
			return (NULL);
		}
		*eq = '\0';
		t_taglist_insert(T, copy, eq + 1);
		free(copy);
	}

	return (T);
}


static bool
t_file_clear(struct t_file *file, const struct t_taglist *T)
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
						free(data->vc->user_comments[i]);
						data->vc->user_comments[i] = NULL;
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
	} else
		count = 0;

	data->vc->comments = count;
	data->vc->user_comments =
	    xrealloc(data->vc->user_comments, (data->vc->comments + 1) * sizeof(*data->vc->user_comments));
	data->vc->comment_lengths =
	    xrealloc(data->vc->comment_lengths, (data->vc->comments + 1) * sizeof(*data->vc->comment_lengths));
	/* vorbis_comment_add() set the last comment to NULL, we do the same */
	data->vc->user_comments[data->vc->comments]   = NULL;
	data->vc->comment_lengths[data->vc->comments] = 0;
	return (true);
}


static bool
t_file_add(struct t_file *file, const struct t_taglist *T)
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

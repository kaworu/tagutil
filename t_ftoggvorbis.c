/*
 * t_ftoggvorbis.c
 *
 * Ogg/Vorbis backend, using libvorbis and libvorbisfile
 */
#include <stdbool.h>
#include <string.h>

/* Vorbis headers */
#define	OV_EXCLUDE_STATIC_CALLBACKS	1
#include "vorbis/vorbisfile.h"
#undef	OV_EXCLUDE_STATIC_CALLBACKS
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
	int	 i;
	struct t_file *file;
	struct OggVorbis_File	*vf;
	struct vorbis_comment	*vc;
	struct t_oggvorbis_data data;

	assert_not_null(path);

	vf = xmalloc(sizeof(struct OggVorbis_File));
	i = ov_fopen(path, vf);
	if (i != 0) {
		/* XXX: check OV_EFAULT or OV_EREAD? */
		free(vf);
		return (NULL);
	}
	vc = ov_comment(vf, -1);
	if (vc == NULL) {
		free(vf);
		return (NULL);
	}

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


static int
fwrite_drain_func(void *fp, const char *data, int len)
{
	size_t s;

	if (fp == NULL)
		return (-1);
	s = fwrite(data, 1, len, fp);
	return (s == 0 ? -1 : s);
}

static bool
t_file_save(struct t_file *file)
{
	ogg_sync_state oy;
	ogg_stream_state ios, oos;
	ogg_page iog, oog;
	vorbis_info vi;
	ogg_packet op, my_vc_packet, *target;
	vorbis_comment vc;
	struct t_oggvorbis_data *data;
	struct sbuf *sb = NULL;
	FILE *rfp = NULL, *wfp = NULL;
	char *buf, *tmp;
	size_t s;
	ogg_int64_t granpos = 0;
	int npacket = 0, prevW = 0;
	enum {
		BUILDING_VC_PACKET, SETUP, START_READING, STREAMS_INITIALIZED,
		READING_HEADERS, READING_DATA, READING_DATA_NEED_FLUSH,
		READING_DATA_NEED_PAGEOUT, E_O_S, WRITE_FINISH, RENAMING,
		DONE_SUCCESS,
	} state;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);
	t_error_clear(file);

	if (!file->dirty)
		return (true);
	data = file->data;

	/*
	 * The following is quite complicated. In order to modify the file's
	 * tag, we have to rewrite the entire file. The Ogg container is divided
	 * into "pages" and "packets" and the algorithm is to replace the SECOND
	 * packet (which contains vorbis_comment) and copy ALL THE OTHERS.
	 */

	state = BUILDING_VC_PACKET;
	/* create the packet holding our vorbis_comment */
	if (vorbis_commentheader_out(data->vc, &my_vc_packet) != 0)
		goto cleanup;
	vorbis_comment_init(&vc);

	state = SETUP;
	/* open files & stuff */
	if ((rfp = fopen(file->path, "r")) == NULL)
		goto cleanup;
	if ((sb = sbuf_new(NULL, NULL, BUFSIZ + 1, SBUF_FIXEDLEN)) == NULL)
		goto cleanup;
	/* open wfp */
	if (xasprintf(&tmp, "%s/.__%s_XXXXXX", t_dirname(file->path), getprogname()) < 0)
		goto cleanup;
	if (mkstemps(tmp, 0) == -1)
		goto cleanup;
	if ((wfp = fopen(tmp, "w")) == NULL)
		goto cleanup;
	sbuf_set_drain(sb, fwrite_drain_func, wfp);
	vorbis_info_init(&vi);
	(void)ogg_sync_init(&oy); /* always return 0 */

	state = START_READING;
	/* main loop: read the input file into buf in order to sync pages out */
	while (state != E_O_S) {
		switch (ogg_sync_pageout(&oy, &iog)) {
		case 0:  /* more data needed or an internal error occurred. */
		case -1: /* stream has not yet captured sync (bytes were skipped). */
			if (feof(rfp)) {
				if (state < READING_DATA)
					goto cleanup;
				state = E_O_S;
			} else {
				if ((buf = ogg_sync_buffer(&oy, BUFSIZ)) == NULL)
					goto cleanup;
				if ((s = fread(buf, sizeof(char), BUFSIZ, rfp)) == -1)
					goto cleanup;
				if (ogg_sync_wrote(&oy, s) == -1)
					goto cleanup;
			}
			break;
		case 1: /* a page was synced and returned. */
			if (npacket == 0) {
				/* init both input and output streams with the serialno of the first page */
				if (ogg_stream_init(&ios, ogg_page_serialno(&iog)) == -1)
					goto cleanup;
				if (ogg_stream_init(&oos, ogg_page_serialno(&iog)) == -1) {
					ogg_stream_clear(&ios);
					goto cleanup;
				}
				state = STREAMS_INITIALIZED;
			}

			/* page in into the input stream, in order to fetch ogg_packet from the page */
			if (ogg_stream_pagein(&ios, &iog) == -1)
				goto cleanup;

			while (ogg_stream_packetout(&ios, &op) == 1) {
				/* the second packet is the commentheader packet, we replace it with my_vc_packet */
				target = (++npacket == 2 ? &my_vc_packet : &op);

				/* insert the target packet into the output stream */
				if (npacket <= 3) {
					/* the three first packet are used to fill the vorbis info (used to compute granule) */
					if (vorbis_synthesis_headerin(&vi, &vc, &op) != 0)
						goto cleanup;
					/* force a flush after the third ogg_packet */
					state = (npacket == 3 ? READING_DATA_NEED_FLUSH : READING_HEADERS);
				} else {
					/* granule computation */
					int bs   = vorbis_packet_blocksize(&vi, &op);
					granpos += (prevW == 0 ? 0 : (bs + prevW) / 4);
					prevW    = bs;

					/* save a page in the buffer a page if needed */
					while ((state == READING_DATA_NEED_FLUSH && ogg_stream_flush(&oos, &oog)) ||
					    (state == READING_DATA_NEED_PAGEOUT && ogg_stream_pageout(&oos, &oog))) {
						(void)sbuf_bcat(sb, oog.header, oog.header_len);
						(void)sbuf_bcat(sb, oog.body, oog.body_len);
					}

					/* flush / out / granpos hack, idk the logic i just saw it in vcedit.c */
					state = READING_DATA;
					if (op.granulepos == -1) {
						op.granulepos = granpos;
					} else if (granpos > op.granulepos) {
						granpos = op.granulepos;
						state = READING_DATA_NEED_FLUSH;
					} else
						state = READING_DATA_NEED_PAGEOUT;
				}
				assert(ogg_stream_packetin(&oos, target) == 0); /* 0 on success, -1 on error */
			}
		}
	}

	/* forces remaining packets into a last page */
	oos.e_o_s = 1;
	while (ogg_stream_flush(&oos, &oog)) {
		(void)sbuf_bcat(sb, oog.header, oog.header_len);
		(void)sbuf_bcat(sb, oog.body, oog.body_len);
	}
	(void)fclose(rfp);
	rfp = NULL;

	state = WRITE_FINISH;
	if (sbuf_finish(sb) == -1)
		goto cleanup;
	if (fclose(wfp) != 0)
		goto cleanup;
	wfp = NULL;

	state = RENAMING;
	if (rename(tmp, file->path) == -1)
		goto cleanup;

	state = DONE_SUCCESS;

cleanup:
	switch (state) {
	case DONE_SUCCESS: /* FALLTHROUGH */
	case RENAMING:
		if (state != DONE_SUCCESS && t_error_msg(file) == NULL)
			t_error_set(file, "error while renaming temporary file");
		/* FALLTHROUGH */
	case WRITE_FINISH: /* FALLTHROUGH */
	case READING_HEADERS: /* FALLTHROUGH */
	case READING_DATA: /* FALLTHROUGH */
	case READING_DATA_NEED_FLUSH: /* FALLTHROUGH */
	case READING_DATA_NEED_PAGEOUT: /* FALLTHROUGH */
	case E_O_S: /* FALLTHROUGH */
	case STREAMS_INITIALIZED:
		ogg_stream_clear(&ios);
		ogg_stream_clear(&oos);
		/* FALLTHROUGH */
	case START_READING:
		if (state != DONE_SUCCESS && t_error_msg(file) == NULL)
			t_error_set(file, "error while reading or writting to temporary file");
		ogg_sync_clear(&oy);
		vorbis_info_clear(&vi);
		/* FALLTHROUGH */
	case SETUP:
		if (state != DONE_SUCCESS && t_error_msg(file) == NULL)
			t_error_set(file, "error while opening");
		if (wfp != NULL) {
			(void)fclose(wfp);
			(void)unlink(tmp);
		}
		free(tmp);
		if (sb != NULL)
			sbuf_delete(sb);
		if (rfp != NULL)
			(void)fclose(rfp);
		ogg_packet_clear(&my_vc_packet);
		/* FALLTHROUGH */
	case BUILDING_VC_PACKET:
		if (state != DONE_SUCCESS && t_error_msg(file) == NULL)
			t_error_set(file, "error while creating vorbis comment ogg_packet");
		vorbis_comment_clear(&vc);
	}

	return (state == DONE_SUCCESS);
}


static struct t_taglist *
t_file_get(struct t_file *file, const char *key)
{
	char	*copy;
	char	*eq;
	int	 i;
	size_t	klen;
	const char *c;
	struct t_taglist *T;
	struct t_oggvorbis_data *data;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);
	t_error_clear(file);

	data = file->data;
	if ((T = t_taglist_new()) == NULL)
		err(ENOMEM, "malloc");
	if (key != NULL)
		klen = strlen(key);

	for (i = 0; i < data->vc->comments; i++) {
		c = data->vc->user_comments[i];
		if (key != NULL) {
			if (strncasecmp(key, c, klen) != 0 || c[klen] != '=')
				continue;
		}
		copy = xstrdup(c);
		eq = strchr(copy, '=');
		if (eq == NULL) {
			t_error_set(file, "ogg/vorbis header seems corrupted");
			free(copy);
			t_taglist_delete(T);
			return (NULL);
		}
		*eq = '\0';
		if ((t_taglist_insert(T, copy, eq + 1)) == -1)
			err(ENOMEM, "malloc");
		free(copy);
	}

	return (T);
}


static bool
t_file_clear(struct t_file *file, const struct t_taglist *T)
{
	int	 i;
	char	*c, *copy, *eq;
	struct t_taglist *backup;
	struct t_tag *t;
	struct t_oggvorbis_data *data;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);
	t_error_clear(file);

	data = file->data;

	/* this is overkill, but libvorbis doesn't expose routine to remove
	  comments. */
	if ((backup = t_taglist_new()) == NULL)
		err(ENOMEM, "malloc");
	if (T != NULL) {
		/* do a backup of the tags we want to keep */
		for (i = 0; i < data->vc->comments; i++) {
			c = data->vc->user_comments[i];
			TAILQ_FOREACH(t, T->tags, entries) {
				if (!(strncasecmp(t->key, c, t->klen) == 0 && c[t->klen] == '=')) {
					copy = xstrdup(c);
					eq = strchr(copy, '=');
					if (eq == NULL) {
						t_error_set(file, "ogg/vorbis header seems corrupted");
						free(copy);
						t_taglist_delete(backup);
						return (NULL);
					}
					*eq = '\0';
					if ((t_taglist_insert(backup, copy, eq + 1)) == -1)
						err(ENOMEM, "malloc");
					free(copy);
				}
			}
		}
	}

	vorbis_comment_clear(data->vc);
	vorbis_comment_init(data->vc);
	t_file_add(file, backup);
	t_taglist_delete(backup);

	return (true);
}


static bool
t_file_add(struct t_file *file, const struct t_taglist *T)
{
	struct t_tag *t;
	struct t_oggvorbis_data *data;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);
	assert_not_null(T);
	t_error_clear(file);

	data = file->data;

	TAILQ_FOREACH(t, T->tags, entries) {
		vorbis_comment_add_tag(data->vc, t->key, t->val);
		file->dirty++;
	}

	return (true);
}

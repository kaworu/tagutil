/*
 * t_ftoggvorbis.c
 *
 * Ogg/Vorbis backend, using libogg, libvorbis and libvorbisfile
 */

/* Ogg headers */
#include "ogg/ogg.h"
/* Vorbis headers */
#define	OV_EXCLUDE_STATIC_CALLBACKS	1
#include "vorbis/vorbisfile.h"
#undef	OV_EXCLUDE_STATIC_CALLBACKS
#include "vorbis/codec.h"

#include "t_config.h"
#include "t_backend.h"
#include "t_error.h"


static const char libid[] = "libvorbis";


struct t_ftoggvorbis_data {
	const char		*libid; /* pointer to libid */
	const char		*path; /* this is needed for t_ftoggvorbis_write() */
	struct OggVorbis_File	 vf;
};


struct t_backend	*t_ftoggvorbis_backend(void);

static void 		*t_ftoggvorbis_init(const char *path);
static struct t_taglist	*t_ftoggvorbis_read(void *opaque);
static int		 t_ftoggvorbis_write(void *opaque, const struct t_taglist *tlist);
static void		 t_ftoggvorbis_clear(void *opaque);

/* helper for t_ftoggvorbis_write() */
static int		 fwrite_drain_func(void *fp, const char *data, int len);


struct t_backend *
t_ftoggvorbis_backend(void)
{
	static struct t_backend b = {
		.libid		= libid,
		.desc		= "Ogg/Vorbis files format",
		.init		= t_ftoggvorbis_init,
		.read		= t_ftoggvorbis_read,
		.write		= t_ftoggvorbis_write,
		.clear		= t_ftoggvorbis_clear,
	};
	return (&b);
}


static void *
t_ftoggvorbis_init(const char *path)
{
	size_t plen;
	char *p;
	struct t_ftoggvorbis_data *data;

	assert_not_null(path);

	plen = strlen(path);
	data = malloc(sizeof(struct t_ftoggvorbis_data) + plen + 1);
	if (data == NULL)
		return (NULL);
	data->libid = libid;
	data->path = p = (char *)(data + 1);
	assert(strlcpy(p, path, plen + 1) == plen);
	bzero(&data->vf, sizeof(struct OggVorbis_File));

	if (ov_fopen(data->path, &data->vf) != 0) {
		/* XXX: check OV_EFAULT or OV_EREAD? */
		free(data);
		return (NULL);
	}

	return (data);
}


static struct t_taglist *
t_ftoggvorbis_read(void *opaque)
{
	struct t_taglist *tlist;
	struct t_ftoggvorbis_data *data;
	struct vorbis_comment *vc;
	int i;

	assert_not_null(opaque);
	data = opaque;
	assert(data->libid == libid);

	vc = ov_comment(&data->vf, -1);
	if (vc == NULL)
		return (NULL);

	tlist = t_taglist_new();
	if (tlist == NULL)
		return (NULL);

	for (i = 0; i < vc->comments; i++) {
		int r;
		char *c  = vc->user_comments[i];
		char *eq = strchr(c, '=');

		if (eq == NULL) {
			warnx("invalid vorbis comment: %s", c);
			continue;
		}
		*eq = '\0';
		r = t_taglist_insert(tlist, c, eq + 1);
		*eq = '=';
		if (r == -1) {
			t_taglist_delete(tlist);
			return (NULL);
		}
	}

	return (tlist);
}


static int
t_ftoggvorbis_write(void *opaque, const struct t_taglist *tlist)
{
	ogg_sync_state oy;
	ogg_stream_state ios, oos;
	ogg_page iog, oog;
	vorbis_info vi;
	ogg_packet op, my_vc_packet, *target;
	vorbis_comment vc;
	struct t_ftoggvorbis_data *data;
	const struct t_tag *t;
	struct t_error e;
	struct sbuf *sb = NULL;
	FILE *rfp = NULL, *wfp = NULL;
	char *buf, *tmp = NULL;
	size_t s;
	ogg_int64_t granpos = 0;
	int npacket = 0, prevW = 0;
	enum {
		BUILDING_VC_PACKET, SETUP, START_READING, STREAMS_INITIALIZED,
		READING_HEADERS, READING_DATA, READING_DATA_NEED_FLUSH,
		READING_DATA_NEED_PAGEOUT, E_O_S, WRITE_FINISH, RENAMING,
		DONE_SUCCESS,
	} state;

	assert_not_null(opaque);
	data = opaque;
	assert(data->libid == libid);

	/*
	 * The following is quite complicated. In order to modify the file's
	 * tag, we have to rewrite the entire file. The Ogg container is divided
	 * into "pages" and "packets" and the algorithm is to replace the SECOND
	 * ogg packet (which contains vorbis comments) and copy ALL THE OTHERS.
	 */

	state = BUILDING_VC_PACKET;
	vorbis_comment_init(&vc);
	TAILQ_FOREACH(t, tlist->tags, entries)
		vorbis_comment_add_tag(&vc, t->key, t->val);
	/* create the packet holding our vorbis_comment */
	if (vorbis_commentheader_out(&vc, &my_vc_packet) != 0)
		goto cleanup;
	vorbis_comment_clear(&vc);
	vorbis_comment_init(&vc);

	state = SETUP;
	/* open files & stuff */
	if ((rfp = fopen(data->path, "r")) == NULL)
		goto cleanup;
	if ((sb = sbuf_new(NULL, NULL, BUFSIZ + 1, SBUF_FIXEDLEN)) == NULL)
		goto cleanup;
	/* open the write file pointer */
	if (asprintf(&tmp, "%s/.__%s_XXXXXX", t_dirname(data->path), getprogname()) < 0)
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
				/*
				 * init both input and output streams with the
				 * serialno of the first page
				 */
				if (ogg_stream_init(&ios, ogg_page_serialno(&iog)) == -1)
					goto cleanup;
				if (ogg_stream_init(&oos, ogg_page_serialno(&iog)) == -1) {
					ogg_stream_clear(&ios);
					goto cleanup;
				}
				state = STREAMS_INITIALIZED;
			}

			/*
			 * page in into the input stream, in order to fetch
			 * ogg_packet from the page
			 */
			if (ogg_stream_pagein(&ios, &iog) == -1)
				goto cleanup;

			while (ogg_stream_packetout(&ios, &op) == 1) {
				/*
				 * the second packet is the commentheader
				 * packet, we replace it with my_vc_packet
				 */
				target = (++npacket == 2 ? &my_vc_packet : &op);

				/* insert the target packet into the output stream */
				if (npacket <= 3) {
					/*
					 * the three first packet are used to
					 * fill the vorbis info (used later for
					 * granule computation)
					 */
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
					while (
					    (state == READING_DATA_NEED_FLUSH && ogg_stream_flush(&oos, &oog)) ||
					    (state == READING_DATA_NEED_PAGEOUT && ogg_stream_pageout(&oos, &oog))) {
						(void)sbuf_bcat(sb, oog.header, oog.header_len);
						(void)sbuf_bcat(sb, oog.body, oog.body_len);
					}

					/*
					 * force a flush or a pageout,  granpos
					 * hack, idk the logic i just saw it in
					 * vcedit.c
					 */
					state = READING_DATA;
					if (op.granulepos == -1) {
						op.granulepos = granpos;
					} else if (granpos > op.granulepos) {
						granpos = op.granulepos;
						state = READING_DATA_NEED_FLUSH;
					} else
						state = READING_DATA_NEED_PAGEOUT;
				}
				if (ogg_stream_packetin(&oos, target) == -1)
					goto cleanup;
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
	if (rename(tmp, data->path) == -1)
		goto cleanup;

	state = DONE_SUCCESS;

cleanup:
	t_error_init(&e);
	switch (state) {
	case DONE_SUCCESS: /* FALLTHROUGH */
	case RENAMING:
		if (state != DONE_SUCCESS && t_error_msg(&e) == NULL)
			t_error_set(&e, "error while renaming temporary file");
		/* FALLTHROUGH */
	case WRITE_FINISH: /* FALLTHROUGH */
	case E_O_S: /* FALLTHROUGH */
	case READING_DATA_NEED_PAGEOUT: /* FALLTHROUGH */
	case READING_DATA_NEED_FLUSH: /* FALLTHROUGH */
	case READING_DATA: /* FALLTHROUGH */
	case READING_HEADERS: /* FALLTHROUGH */
	case STREAMS_INITIALIZED:
		ogg_stream_clear(&ios);
		ogg_stream_clear(&oos);
		/* FALLTHROUGH */
	case START_READING:
		if (state != DONE_SUCCESS && t_error_msg(&e) == NULL)
			t_error_set(&e, "error while reading or writting to temporary file");
		ogg_sync_clear(&oy);
		vorbis_info_clear(&vi);
		/* FALLTHROUGH */
	case SETUP:
		if (state != DONE_SUCCESS && t_error_msg(&e) == NULL)
			t_error_set(&e, "error while opening");
		if (wfp != NULL)
			(void)fclose(wfp);
		if (tmp != NULL && eaccess(tmp, R_OK) != -1)
			(void)unlink(tmp);
		free(tmp);
		if (sb != NULL)
			sbuf_delete(sb);
		if (rfp != NULL)
			(void)fclose(rfp);
		ogg_packet_clear(&my_vc_packet);
		/* FALLTHROUGH */
	case BUILDING_VC_PACKET:
		if (state != DONE_SUCCESS && t_error_msg(&e) == NULL)
			t_error_set(&e, "error while creating vorbis comment ogg_packet");
		vorbis_comment_clear(&vc);
	}

	/*
	 * XXX: at the moment, we ignore the error message
	 */
	t_error_clear(&e);
	return (state == DONE_SUCCESS ? 0 : -1);
}


static void
t_ftoggvorbis_clear(void *opaque)
{
	struct t_ftoggvorbis_data *data;

	assert_not_null(opaque);
	data = opaque;
	assert(data->libid == libid);

	ov_clear(&data->vf);
	free(data);
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

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

/* helpers for t_ftoggvorbis_write() */
static int		 fwrite_drain_func(void *fp, const char *data, int len);
static int		 sbuf_write_ogg_page(struct sbuf *sb, ogg_page *p);


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
	(void)memcpy(p, path, plen + 1);
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
	FILE             *fp_in  = NULL;  /* input file pointer */
	FILE             *fp_out = NULL; /* output file pointer */
	ogg_sync_state    oy_in;  /* sync and verify incoming physical bitstream */
	ogg_stream_state  os_in;  /* take physical pages, weld into a logical
	                             stream of packets */
	ogg_stream_state  os_out; /* take physical pages, weld into a logical
	                             stream of packets */
	ogg_page          og_in;  /* one Ogg bitstream page. Vorbis packets are inside */
	ogg_page          og_out; /* one Ogg bitstream page. Vorbis packets are inside */
	ogg_packet        op_in;  /* one raw packet of data for decode */
	ogg_packet        my_vc_packet; /* our custom packet containing vc_out */
	vorbis_info       vi_in;  /* struct that stores all the static vorbis
	                             bitstream settings */
	vorbis_comment    vc_in;  /* struct that stores all the bitstream user
	                             comment */
	vorbis_comment    vc_out; /* struct that stores all the bitstream user
	                             comment */
	unsigned long     nstream_in; /* stream counter */
	unsigned long     npage_in;   /* page counter */
	unsigned long     npacket_in; /* packet counter */
	unsigned long     bs;     /* blocksize of the current packet */
	unsigned long     lastbs; /* blocksize of the last packet */
	ogg_int64_t       granulepos; /* granulepos of the current page */
	struct sbuf *sb = NULL;
	char *tempfile  = NULL;
	struct t_ftoggvorbis_data *data;
	const struct t_tag *t;
	enum {
		BUILDING_VC_PACKET, SETUP, B_O_S, START_READING,
		STREAMS_INITIALIZED, READING_HEADERS, READING_DATA,
		READING_DATA_NEED_FLUSH, READING_DATA_NEED_PAGEOUT, E_O_S,
		WRITE_FINISH, RENAMING, DONE_SUCCESS,
	} state;

	assert_not_null(opaque);
	data = opaque;
	assert(data->libid == libid);

	/*
	 * In order to modify the file's tag, we have to rewrite the entire
	 * file. The Ogg container is divided into "pages" and "packets" and the
	 * algorithm is to replace the SECOND ogg packet (which contains vorbis
	 * comments) and copy ALL THE OTHERS. See "Metadata workflow":
	 * https://xiph.org/vorbis/doc/libvorbis/overview.html
	 */

	state = BUILDING_VC_PACKET;
	vorbis_comment_init(&vc_out);
	TAILQ_FOREACH(t, tlist->tags, entries)
		vorbis_comment_add_tag(&vc_out, t->key, t->val);
	/* create the packet holding our vorbis_comment */
	if (vorbis_commentheader_out(&vc_out, &my_vc_packet) != 0)
		goto cleanup_label;
	vorbis_comment_clear(&vc_out);

	state = SETUP;
	/* open files & stuff */
	(void)ogg_sync_init(&oy_in); /* always return 0 */
	if ((fp_in = fopen(data->path, "r")) == NULL)
		goto cleanup_label;
	if ((sb = sbuf_new(NULL, NULL, BUFSIZ + 1, SBUF_FIXEDLEN)) == NULL)
		goto cleanup_label;
	/* open the write file pointer */
	if (asprintf(&tempfile, "%s/.__%s_XXXXXX", t_dirname(data->path), getprogname()) < 0)
		goto cleanup_label;
	if (mkstemps(tempfile, 0) == -1)
		goto cleanup_label;
	if ((fp_out = fopen(tempfile, "w")) == NULL)
		goto cleanup_label;
	sbuf_set_drain(sb, fwrite_drain_func, fp_out);
	lastbs = granulepos = 0;

	nstream_in = 0;
bos_label: /* beginning of a stream */
	state = B_O_S; /* never read, but that's fine */
	nstream_in += 1;
	npage_in = npacket_in = 0;
	vorbis_info_init(&vi_in);
	vorbis_comment_init(&vc_in);

	state = START_READING;
	/* main loop: read the input file into buf in order to sync pages out */
	while (state != E_O_S) {
		switch (ogg_sync_pageout(&oy_in, &og_in)) {
		case 0:  /* more data needed or an internal error occurred. */
		case -1: /* stream has not yet captured sync (bytes were skipped). */
			if (feof(fp_in)) {
				if (state < READING_DATA)
					goto cleanup_label;
				/* There is no more data to read and we could
				   not get a page so we're done here. */
				state = E_O_S;
			} else {
				/* read more data and try again to get a page. */
				char *buf;
				size_t s;
				/* get a buffer */
				if ((buf = ogg_sync_buffer(&oy_in, BUFSIZ)) == NULL)
					goto cleanup_label;
				/* read a part of the file */
				if ((s = fread(buf, sizeof(char), BUFSIZ, fp_in)) == -1)
					goto cleanup_label;
				/* tell ogg how much was read */
				if (ogg_sync_wrote(&oy_in, s) == -1)
					goto cleanup_label;
			}
			continue;
		}
		/* here ogg_sync_pageout() returned 1 and a page was sync'ed. */
		if (++npage_in == 1) {
			/* init both input and output streams with the serialno
			   of the first page */
			if (ogg_stream_init(&os_in, ogg_page_serialno(&og_in)) == -1)
				goto cleanup_label;
			if (ogg_stream_init(&os_out, ogg_page_serialno(&og_in)) == -1) {
				ogg_stream_clear(&os_in);
				goto cleanup_label;
			}
			state = STREAMS_INITIALIZED;
		}

		/* put the page in input stream, and then loop through each its
		   packet(s) */
		if (ogg_stream_pagein(&os_in, &og_in) == -1)
			goto cleanup_label;
		while (ogg_stream_packetout(&os_in, &op_in) == 1) {
			ogg_packet *target;
			/*
			 * This is where we really do what we mean to do: the
			 * second packet is the commentheader packet, we replace
			 * it with my_vc_packet if we're on the first stream.
			 */
			if (++npacket_in == 2 && nstream_in == 1)
				target = &my_vc_packet;
			else
				target = &op_in;

			if (npacket_in <= 3) {
				/*
				 * The first three packets are header packets.
				 * We use them to get the vorbis_info which
				 * will be used later. vc_in will not be unused.
				 */
				if (vorbis_synthesis_headerin(&vi_in, &vc_in, &op_in) != 0)
					goto cleanup_label;
				/* force a flush after the third ogg_packet */
				state = (npacket_in == 3 ? READING_DATA_NEED_FLUSH : READING_HEADERS);
			} else {
				/*
				 * granulepos computation.
				 *
				 * The granulepos is stored into the *pages* and
				 * is used by the codec to seek through the
				 * bitstream.  Its value is codec dependent (in
				 * the Vorbis case it is the number of samples
				 * elapsed).
				 *
				 * The vorbis_packet_blocksize() actually
				 * compute the number of sample that would be
				 * stored by the packet (without decoding it).
				 * This is the same formula as in vcedit example
				 * from vorbis-tools.
				 *
				 * We use here the vorbis_info previously filled
				 * when reading header packets.
				 *
				 * XXX: check if this is not a vorbis stream ?
				 */
				bs = vorbis_packet_blocksize(&vi_in, &op_in);
				granulepos += (lastbs == 0 ? 0 : (bs + lastbs) / 4);
				lastbs = bs;

				/* write page(s) if needed */
				if (state == READING_DATA_NEED_FLUSH) {
					while (ogg_stream_flush(&os_out, &og_out))
						sbuf_write_ogg_page(sb, &og_out);
				} else if (state == READING_DATA_NEED_PAGEOUT) {
					while (ogg_stream_pageout(&os_out, &og_out))
						sbuf_write_ogg_page(sb, &og_out);
				}

				/*
				 * Decide wether we need to write a page based
				 * on our granulepos computation. The -1 case is
				 * very common because only the last packet of a
				 * page has its granulepos set by the ogg layer
				 * (which only store a granulepos per page), so
				 * all the other have a value of -1 (we need to
				 * set the granulepos for each packet though).
				 *
				 * The other cases logic are borrowed from
				 * vcedit and I fail to see how granulepos could
				 * mismatch because we don't alter the data
				 * packet.
				 */
				state = READING_DATA;
				if (op_in.granulepos == -1) {
					op_in.granulepos = granulepos;
				} else if (granulepos <= op_in.granulepos) {
					state = READING_DATA_NEED_PAGEOUT;
				} else /* if granulepos > op_in.granulepos */ {
					state = READING_DATA_NEED_FLUSH;
					granulepos = op_in.granulepos;
				}
			}
			/* insert the target packet into the output stream */
			if (ogg_stream_packetin(&os_out, target) == -1)
				goto cleanup_label;
		}
		if (ogg_page_eos(&og_in)) {
			/* og_in was the last page of the stream */
			state = E_O_S;
		}
	}

	/* forces remaining packets into a last page */
	os_out.e_o_s = 1;
	while (ogg_stream_flush(&os_out, &og_out))
		sbuf_write_ogg_page(sb, &og_out);
	/* ogg_page and ogg_packet structs always point to storage in libvorbis.
	   They're never freed or manipulated directly */

	/* check if we need to read another stream */
	if (!feof(fp_in)) {
		ogg_stream_clear(&os_in);
		ogg_stream_clear(&os_out);
		vorbis_comment_clear(&vc_in);
		vorbis_info_clear(&vi_in);
		goto bos_label;
	} else {
		(void)fclose(fp_in);
		fp_in = NULL;
	}

	state = WRITE_FINISH;
	if (sbuf_finish(sb) == -1)
		goto cleanup_label;
	if (fclose(fp_out) != 0)
		goto cleanup_label;
	fp_out = NULL;

	state = RENAMING;
	if (rename(tempfile, data->path) == -1)
		goto cleanup_label;

	state = DONE_SUCCESS;
cleanup_label:
	if (state >= STREAMS_INITIALIZED) {
		ogg_stream_clear(&os_in);
		ogg_stream_clear(&os_out);
	}
	if (state >= START_READING) {
		vorbis_comment_clear(&vc_in);
		vorbis_info_clear(&vi_in);
	}
	ogg_sync_clear(&oy_in);
	if (fp_out != NULL)
		(void)fclose(fp_out);
	if (tempfile != NULL && eaccess(tempfile, R_OK) != -1)
		(void)unlink(tempfile);
	free(tempfile);
	if (sb != NULL)
		sbuf_delete(sb);
	if (fp_in != NULL)
		(void)fclose(fp_in);
	ogg_packet_clear(&my_vc_packet);
	vorbis_comment_clear(&vc_out);

	/*
	 * FIXME: at the moment, we don't warn about the problem.
	 */
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


static int
sbuf_write_ogg_page(struct sbuf *sb, ogg_page *og)
{
	int ret = 0;

	assert_not_null(sb);
	assert_not_null(og);

	ret += sbuf_bcat(sb, og->header, og->header_len);
	ret += sbuf_bcat(sb, og->body,   og->body_len);

	return (ret == 0 ? 0 : -1);
}

/*
 * t_ftid3v1.c
 *
 * ID3v1 backend.
 */
#include <stdio.h>

#include "t_config.h"
#include "t_backend.h"


static const char libid[] = "ID3v1";

static const char * const id3v1_genre_str[] = {
      [0] = "Blues",
      [1] = "Classic Rock",
      [2] = "Country",
      [3] = "Dance",
      [4] = "Disco",
      [5] = "Funk",
      [6] = "Grunge",
      [7] = "Hip-Hop",
      [8] = "Jazz",
      [9] = "Metal",
     [10] = "New Age",
     [11] = "Oldies",
     [12] = "Other",
     [13] = "Pop",
     [14] = "R&B",
     [15] = "Rap",
     [16] = "Reggae",
     [17] = "Rock",
     [18] = "Techno",
     [19] = "Industrial",
     [20] = "Alternative",
     [21] = "Ska",
     [22] = "Death Metal",
     [23] = "Pranks",
     [24] = "Soundtrack",
     [25] = "Euro-Techno",
     [26] = "Ambient",
     [27] = "Trip-Hop",
     [28] = "Vocal",
     [29] = "Jazz+Funk",
     [30] = "Fusion",
     [31] = "Trance",
     [32] = "Classical",
     [33] = "Instrumental",
     [34] = "Acid",
     [35] = "House",
     [36] = "Game",
     [37] = "Sound Clip",
     [38] = "Gospel",
     [39] = "Noise",
     [40] = "AlternRock",
     [41] = "Bass",
     [42] = "Soul",
     [43] = "Punk",
     [44] = "Space",
     [45] = "Meditative",
     [46] = "Instrumental Pop",
     [47] = "Instrumental Rock",
     [48] = "Ethnic",
     [49] = "Gothic",
     [50] = "Darkwave",
     [51] = "Techno-Industrial",
     [52] = "Electronic",
     [53] = "Pop-Folk",
     [54] = "Eurodance",
     [55] = "Dream",
     [56] = "Southern Rock",
     [57] = "Comedy",
     [58] = "Cult",
     [59] = "Gangsta",
     [60] = "Top 40",
     [61] = "Christian Rap",
     [62] = "Pop/Funk",
     [63] = "Jungle",
     [64] = "Native American",
     [65] = "Cabaret",
     [66] = "New Wave",
     [67] = "Psychadelic", /* gloup */
     [68] = "Rave",
     [69] = "Showtunes",
     [70] = "Trailer",
     [71] = "Lo-Fi",
     [72] = "Tribal",
     [73] = "Acid Punk",
     [74] = "Acid Jazz",
     [75] = "Polka",
     [76] = "Retro",
     [77] = "Musical",
     [78] = "Rock & Roll",
     [79] = "Hard Rock",
     [80] = "Folk",
     [81] = "Folk-Rock",
     [82] = "National Folk",
     [83] = "Swing",
     [84] = "Fast Fusion",
     [85] = "Bebob",
     [86] = "Latin",
     [87] = "Revival",
     [88] = "Celtic",
     [89] = "Bluegrass",
     [90] = "Avantgarde",
     [91] = "Gothic Rock",
     [92] = "Progressive Rock",
     [93] = "Psychedelic Rock",
     [94] = "Symphonic Rock",
     [95] = "Slow Rock",
     [96] = "Big Band",
     [97] = "Chorus",
     [98] = "Easy Listening",
     [99] = "Acoustic",
    [100] = "Humour",
    [101] = "Speech",
    [102] = "Chanson",
    [103] = "Opera",
    [104] = "Chamber Music",
    [105] = "Sonata",
    [106] = "Symphony",
    [107] = "Booty Bass",
    [108] = "Primus",
    [109] = "Porn Groove",
    [110] = "Satire",
    [111] = "Slow Jam",
    [112] = "Club",
    [113] = "Tango",
    [114] = "Samba",
    [115] = "Folklore",
    [116] = "Ballad",
    [117] = "Power Ballad",
    [118] = "Rhythmic Soul",
    [119] = "Freestyle",
    [120] = "Duet",
    [121] = "Punk Rock",
    [122] = "Drum Solo",
    [123] = "A capella",
    [124] = "Euro-House",
    [125] = "Dance Hall",
    [126] = "Goa",
    [127] = "Drum & Bass",
    [128] = "Club-House",
    [129] = "Hardcore",
    [130] = "Terror",
    [131] = "Indie",
    [132] = "BritPop",
    [133] = "Negerpunk",
    [134] = "Polsk Punk",
    [135] = "Beat",
    [136] = "Christian Gangsta",
    [137] = "Heavy Metal",
    [138] = "Black Metal",
    [139] = "Crossover",
    [140] = "Contemporary Christian",
    [141] = "Christian Rock",
    [142] = "Merengue",
    [143] = "Salsa",
    [144] = "Thrash Metal",
    [145] = "Anime",
    [146] = "JPop",
    [147] = "SynthPop",
};


struct id3v1_tag {
	char	magic[3];
	char	title[30];
	char	artist[30];
	char	album[30];
	char	year[4];
	union {
		struct { /* ID3v1.0 */
			char	comment[30];
		} v1_0;
		struct { /* ID3v1.1 */
			char	comment[28];
			unsigned char	has_tracknumber;
			unsigned char	tracknumber;
		} v1_1;
	} comment;
	unsigned char	genre;
} t__packed;

struct t_ftid3v1_data {
	const char	*libid; /* pointer to libid */
	const char	*path; /* this is needed for t_ftid3v1_write() */
	int		 id3; /* 1 if id3 tag is already present in the file, 0 otherwise */
	FILE		*fp; /* read-only filedescriptor */
};


struct t_backend	*t_ftid3v1_backend(void);

static void 		*t_ftid3v1_init(const char *path);
static struct t_taglist	*t_ftid3v1_read(void *opaque);
static int		 t_ftid3v1_write(void *opaque, const struct t_taglist *tlist);
static void		 t_ftid3v1_clear(void *opaque);

static int		 id3tag_to_taglist(const struct id3v1_tag *tag, struct t_taglist *tlist);
static int		 taglist_to_id3tag(const struct t_taglist *tlist, struct id3v1_tag *tag);

struct t_backend *
t_ftid3v1_backend(void)
{

	assert(sizeof(struct id3v1_tag) == 128);

	static struct t_backend b = {
		.libid		= libid,
		.desc		= "ID3v1 tag used by old mp3 files",
		.init		= t_ftid3v1_init,
		.read		= t_ftid3v1_read,
		.write		= t_ftid3v1_write,
		.clear		= t_ftid3v1_clear,
	};

	return (&b);
}


static void *
t_ftid3v1_init(const char *path)
{
	char magic[3], *p;
	struct t_ftid3v1_data *data = NULL;
	FILE *fp = NULL;
	size_t plen;

	assert_not_null(path);

	plen = strlen(path);
	data = malloc(sizeof(struct t_ftid3v1_data) + plen + 1);
	if (data == NULL)
		goto error;
	data->libid = libid;
	data->path = p = (char *)(data + 1);
	assert(strlcpy(p, path, plen + 1) == plen);

	if ((data->fp = fp = fopen(data->path, "r")) == NULL)
		goto error;
	if (fseek(fp, -(sizeof(struct id3v1_tag)), SEEK_END) != 0)
		goto error;
	if (fread(&magic, sizeof(char), countof(magic), fp) != countof(magic))
		goto error;
	data->id3 = (
	    magic[0] == 'T' &&
	    magic[1] == 'A' &&
	    magic[2] == 'G' ?
	    1 : 0
	);

	if (fseek(fp, 0L, SEEK_SET) != 0)
		goto error;
	if (fread(&magic, sizeof(char), countof(magic), fp) != countof(magic))
		goto error;
	if (data->id3) {
		/* check that we don't handle a file with ID3v2 tags. */
#if 0
		if (magic[0] == 'I' &&
		    magic[1] == 'D' &&
		    magic[2] == '3')
			goto error;
#endif
	} else {
		/* check that the file looks like mp3 */
	if (!(magic[0] == 0xFF && magic[1] == 0xFB))
		goto error;
	}

	return (data);
	/* NOTREACHED */
error:
	free(data);
	if (fp != NULL)
		(void)fclose(fp);
	return (NULL);
}


static struct t_taglist *
t_ftid3v1_read(void *opaque)
{
	struct id3v1_tag id3tag;
	struct t_taglist *tlist = NULL;
	struct t_ftid3v1_data *data;

	assert_not_null(opaque);
	data = opaque;
	assert(data->libid == libid);
	assert_not_null(data->fp);

	tlist = t_taglist_new();
	if (tlist == NULL)
		goto error;

	if (!data->id3)
		return (tlist);

	if (fseek(data->fp, -(sizeof(struct id3v1_tag)), SEEK_END) != 0)
		goto error;
	if (fread(&id3tag, sizeof(struct id3v1_tag), 1, data->fp) != 1)
		goto error;

	if (id3tag_to_taglist(&id3tag, tlist) != 0)
		goto error;

	return (tlist);
error:
	t_taglist_delete(tlist);
	return (NULL);
}


static int
t_ftid3v1_write(void *opaque, const struct t_taglist *tlist)
{
	struct id3v1_tag id3tag;
	struct t_ftid3v1_data *data;
	FILE *fp = NULL;

	assert_not_null(opaque);
	assert_not_null(tlist);
	data = opaque;
	assert(data->libid == libid);
	assert_not_null(data->fp);

	if (taglist_to_id3tag(tlist, &id3tag) != 0)
		return (-1);

	(void)fclose(data->fp);
	data->fp = NULL;
	if ((fp = fopen(data->path, "w")) == NULL)
		goto error;

	if (data->id3) {
		if ((fp = fopen(data->path, "w")) == NULL)
			goto error;
		if (fseek(fp, -(sizeof(struct id3v1_tag)), SEEK_END) != 0)
			goto error;
	} else {
		if ((fp = fopen(data->path, "a")) == NULL)
			goto error;
	}
	if (fwrite(&id3tag, sizeof(struct id3v1_tag), 1, fp) != 1)
		goto error;
	if (fclose(fp) != 0)
		goto error;
	fp = NULL;

	data->fp = fopen(data->path, "r");
	return (0);
error:
	if (fp != NULL);
		(void)fclose(fp);
	data->fp = fopen(data->path, "r");
	return (-1);
}


static void
t_ftid3v1_clear(void *opaque)
{
	struct t_ftid3v1_data *data;

	assert_not_null(opaque);
	data = opaque;
	assert(data->libid == libid);

	if (data->fp != NULL)
		(void)fclose(data->fp);
	free(data);
}


static int
id3tag_to_taglist(const struct id3v1_tag *tag, struct t_taglist *tlist)
{
	char buf[31];

	assert_not_null(tag);
	assert_not_null(tlist);

	if (tag->magic[0] != 'T' ||
	    tag->magic[1] != 'A' ||
	    tag->magic[2] != 'G')
		return (-1);

	/* title */
	(void)memcpy(buf, tag->title, 30);
	buf[30] = '\0';
	if (strlen(buf) > 0) {
		if (t_taglist_insert(tlist, "title", buf) != 0)
			return (-1);
	}
	/* artist */
	(void)memcpy(buf, tag->artist, 30);
	buf[30] = '\0';
	if (strlen(buf) > 0) {
		if (t_taglist_insert(tlist, "artist", buf) != 0)
			return (-1);
	}
	/* album */
	(void)memcpy(buf, tag->album, 30);
	buf[30] = '\0';
	if (strlen(buf) > 0) {
		if (t_taglist_insert(tlist, "album", buf) != 0)
			return (-1);
	}
	/* year */
	(void)memcpy(buf, tag->year, 4);
	buf[4] = '\0';
	if (strlen(buf) > 0) {
		if (t_taglist_insert(tlist, "year", buf) != 0)
			return (-1);
	}
	/* comment & tracknumber */
	if (tag->comment.v1_1.has_tracknumber == 0x00) {
		(void)memcpy(buf, tag->comment.v1_1.comment, 28);
		buf[28] = '\0';
		if (strlen(buf) > 0) {
			if (t_taglist_insert(tlist, "comment", buf) != 0)
				return (-1);
		}
		if (tag->comment.v1_1.tracknumber > 0) {
			sprintf(buf, "%d", (int)tag->comment.v1_1.tracknumber);
			if (strlen(buf) > 0) {
				if (t_taglist_insert(tlist, "tracknumber", buf) != 0)
					return (-1);
			}
		}
	} else {
		(void)memcpy(buf, tag->comment.v1_0.comment, 30);
		buf[30] = '\0';
		if (strlen(buf) > 0) {
			if (t_taglist_insert(tlist, "comment", buf) != 0)
				return (-1);
		}
	}
	/* genre */
	if (tag->genre < countof(id3v1_genre_str)) {
		if (t_taglist_insert(tlist, "genre", id3v1_genre_str[tag->genre]) != 0)
			return (-1);
	}

	return (0);
}


static int
taglist_to_id3tag(const struct t_taglist *tlist,struct id3v1_tag *tag)
{

	assert_not_null(tlist);
	assert_not_null(tag);

	warnx("taglist_to_id3tag: not implemented yet.");

	return (-1);
}

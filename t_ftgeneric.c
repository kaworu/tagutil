/*
 * t_ftgeneric.c
 *
 * a generic tagutil backend, using TagLib.
 */
#include <limits.h>

/* TagLib headers */
#include "tag_c.h"

#include "t_config.h"
#include "t_backend.h"


static const char libid[] = "TagLib";


struct t_ftgeneric_data {
	const char	*libid;
	TagLib_File	*file;
	TagLib_Tag	*tag;
};

struct t_backend	*t_ftgeneric_backend(void);

static void 		*t_ftgeneric_init(const char *path);
static struct t_taglist	*t_ftgeneric_read(void *opaque);
static int		 t_ftgeneric_write(void *opaque, const struct t_taglist *tlist);
static void		 t_ftgeneric_clear(void *opaque);


struct t_backend *
t_ftgeneric_backend(void)
{

	static struct t_backend b = {
		.libid		= libid,
		.desc		= "various file format (flac,ogg,mp3...)",
		.init		= t_ftgeneric_init,
		.read		= t_ftgeneric_read,
		.write		= t_ftgeneric_write,
		.clear		= t_ftgeneric_clear,
	};

	/* TagLib specific init */

	/*
	 * FIXME: UTF-8 is a default in TagLib's C API, so this is useless.
	 * However it is unclear when we should switch to Latin1, in particular
	 * when we can't get a locale. This OK for now since most backend assume
	 * an UTF-8 (which should be fixed).
	 */
	char *lc_all, *dot;
	lc_all = getenv("LC_ALL");
	if (lc_all != NULL) {
		dot = strchr(lc_all, '.');
		if (dot != NULL && strcmp(dot + 1, "UTF-8") == 0)
			taglib_set_strings_unicode(true);
	}
	taglib_set_string_management_enabled(false);

	return (&b);
}

static void *
t_ftgeneric_init(const char *path)
{
	TagLib_File *f;
	struct t_ftgeneric_data *data;

	assert_not_null(path);

	data = calloc(1, sizeof(struct t_ftgeneric_data));
	if (data == NULL)
		return (NULL);
	data->libid = libid;

	f = taglib_file_new(path);
	if (f == NULL || !taglib_file_is_valid(f)) {
		free(data);
		return (NULL);
	}

	data->file = f;
	data->tag  = taglib_file_tag(f);

	return (data);
}

static struct t_taglist *
t_ftgeneric_read(void *opaque)
{
	unsigned int uintval;
	char buf[5], *val = NULL;
	struct t_ftgeneric_data *data;
	struct t_taglist *tlist = NULL;

	assert_not_null(opaque);
	data = opaque;
	assert(data->libid == libid);

	if ((tlist = t_taglist_new()) == NULL)
		return (NULL);

	/*
	 * we're abusing assert, mainly because TagLib does not document what
	 * happen on error. There is a lot of duplication but it's ok because
	 * it's very dumb code.
	 */

	assert(val = taglib_tag_title(data->tag));
	if (strlen(val) > 0 && t_taglist_insert(tlist, "title", val) == -1)
		goto error;
	taglib_free(val);
	val = NULL;

	assert(val = taglib_tag_artist(data->tag));
	if (strlen(val) > 0 && t_taglist_insert(tlist, "artist", val) == -1)
		goto error;
	taglib_free(val);
	val = NULL;

	uintval = taglib_tag_year(data->tag);
	if (uintval > 0 && uintval < 10000) {
		if (sprintf(buf, "%04u", uintval) < 0)
			goto error;
		if (t_taglist_insert(tlist, "year", buf) == -1)
			goto error;
	}

	assert(val = taglib_tag_album(data->tag));
	if (strlen(val) > 0 && t_taglist_insert(tlist, "album", val) == -1)
		goto error;
	taglib_free(val);
	val = NULL;

	uintval = taglib_tag_track(data->tag);
	if (uintval > 0 && uintval < 10000) {
		if (sprintf(buf, "%02u", uintval) < 0)
			goto error;
		if (t_taglist_insert(tlist, "track", buf) == -1)
			goto error;
	}

	assert(val = taglib_tag_genre(data->tag));
	if (strlen(val) > 0 && t_taglist_insert(tlist, "genre", val) == -1)
		goto error;
	taglib_free(val);
	val = NULL;

	assert(val = taglib_tag_comment(data->tag));
	if (strlen(val) > 0 && t_taglist_insert(tlist, "comment", val) == -1)
		goto error;
	taglib_free(val);
	val = NULL;

	return (tlist);
error:
	if (val != NULL)
		taglib_free(val);
	t_taglist_delete(tlist);
	return (NULL);
}

static int
t_ftgeneric_write(void *opaque, const struct t_taglist *tlist)
{
	struct t_ftgeneric_data *data;
	struct t_tag *t;
	char *endptr;
	unsigned long ulongval;

	assert_not_null(opaque);
	data = opaque;
	assert(data->libid == libid);

	/* clear all the tags */
	taglib_tag_set_title(data->tag, "");
	taglib_tag_set_artist(data->tag, "");
	taglib_tag_set_year(data->tag, 0);
	taglib_tag_set_album(data->tag, "");
	taglib_tag_set_track(data->tag, 0);
	taglib_tag_set_genre(data->tag, "");
	taglib_tag_set_comment(data->tag, "");

	/* load the tlist */
	TAILQ_FOREACH(t, tlist->tags, entries) {
		if (t_tag_keycmp(t->key, "title") == 0)
			taglib_tag_set_title(data->tag, t->val);
		else if (t_tag_keycmp(t->key, "artist") == 0)
			taglib_tag_set_artist(data->tag, t->val);
		else if (t_tag_keycmp(t->key, "year") == 0) {
			ulongval = strtoul(t->val, &endptr, 10);
			if (endptr == t->val || *endptr != '\0') {
				warnx("invalid unsigned int argument for %s: %s",
				    t->key, t->val);
			} else if (ulongval > UINT_MAX) {
				warnx("invalid unsigned int argument for %s: %s (too large)",
				    t->key, t->val);
			} else if (errno) {
				/* should be EINVAL (ERANGE catched by last condition). */
				assert_fail();
			} else
				taglib_tag_set_year(data->tag, (unsigned int)ulongval);
		} else if (t_tag_keycmp(t->key, "album") == 0)
			taglib_tag_set_album(data->tag, t->val);
		else if (t_tag_keycmp(t->key, "track") == 0) {
			ulongval = strtoul(t->val, &endptr, 10);
			if (endptr == t->val || *endptr != '\0') {
				warnx("invalid unsigned int argument for %s: %s",
				    t->key, t->val);
			} else if (ulongval > UINT_MAX) {
				warnx("invalid unsigned int argument for %s: %s (too large)",
				    t->key, t->val);
			} else if (errno) {
				/* should be EINVAL (ERANGE catched by last condition). */
				assert_fail();
			} else
				taglib_tag_set_track(data->tag, (unsigned int)ulongval);
		} else if (t_tag_keycmp(t->key, "genre") == 0)
			taglib_tag_set_genre(data->tag, t->val);
		else if (t_tag_keycmp(t->key, "comment") == 0)
			taglib_tag_set_comment(data->tag, "");
		else
			warnx("unsupported tag for TagLib backend: %s", t->key);
	}

	if (!taglib_file_save(data->file))
		return (-1);
	return (0);
}

static void
t_ftgeneric_clear(void *opaque)
{
	struct t_ftgeneric_data *data;

	assert_not_null(opaque);
	data = opaque;
	assert(data->libid == libid);

	taglib_file_free(data->file);
	free(data);
}

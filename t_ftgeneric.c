/*
 * t_ftgeneric.c
 *
 * a generic tagutil backend, using TagLib.
 */
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

/* TagLib headers */
#include "tag_c.h"

#include "t_config.h"
#include "t_toolkit.h"
#include "t_file.h"
#include "t_backend.h"


static const char libid[] = "TagLib";
static bool taglib_init_done = false;


struct t_generic_data {
	TagLib_File	*file;
	TagLib_Tag	*tag;
};

static void	taglib_init(void);

_t__nonnull(1)
static struct t_file *	t_file_new(const char *restrict path);

_t__nonnull(1)
static void	t_file_destroy(struct t_file *restrict file);

_t__nonnull(1)
static bool	t_file_save(struct t_file *restrict file);

_t__nonnull(1)
static struct t_taglist * t_file_get(struct t_file *restrict file,
    const char *restrict key);

_t__nonnull(1)
static bool	t_file_clear(struct t_file *restrict file,
    const struct t_taglist *restrict T);

_t__nonnull(1) _t__nonnull(2)
static bool	t_file_add(struct t_file *restrict file,
    const struct t_taglist *restrict T);


void
taglib_init(void)
{
	char *lcall, *dot;

	/* TagLib specific init */
	lcall = getenv("LC_ALL");
	if (lcall != NULL) {
		dot = strchr(lcall, '.');
		if (dot && strcmp(dot + 1, "UTF-8"))
			taglib_set_strings_unicode(true);
	}

	taglib_set_string_management_enabled(false);
	taglib_init_done = true;
}


struct t_backend *
t_generic_backend(void)
{
	static struct t_backend b = {
		.libid		= libid,
    		.desc		= "multiple file format (flac,ogg,mp3...), "
		    "can handle only a limited set of tags.",
		.ctor		= t_file_new,
	};
	return (&b);
}


static struct t_file *
t_file_new(const char *restrict path)
{
	TagLib_File	*f;
	struct t_file *file;
	struct t_generic_data data;

	assert_not_null(path);

	if (!taglib_init_done)
		taglib_init();

	f = taglib_file_new(path);
	if (f == NULL || !taglib_file_is_valid(f))
		return (NULL);

	data.file = f;
	data.tag  = taglib_file_tag(f);

	T_FILE_NEW(file, path, data);
	return (file);
}


static void
t_file_destroy(struct t_file *restrict file)
{
	struct t_generic_data *data;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);

	data = file->data;
	taglib_file_free(data->file);
	t_error_clear(file);
	freex(file);
}


static bool
t_file_save(struct t_file *restrict file)
{
	bool ok;
	struct t_generic_data *data;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);
	t_error_clear(file);

	data = file->data;
	if (!file->dirty)
		return (true);
	ok = taglib_file_save(data->file);
	if (!ok)
		t_error_set(file, "%s error: taglib_file_save", file->libid);
	else
		file->dirty = T_FILE_CLEAN;
	return (ok);
}


static const char * taglibkeys[] = {
	"album", "artist", "description", "date", "genre", "title", "tracknumber"
};
static struct t_taglist *
t_file_get(struct t_file *restrict file, const char *restrict key)
{
	int		 i;
	unsigned int	 uintval;
	char		*value;
	struct t_generic_data *data;
	struct t_taglist *T;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);
	t_error_clear(file);

	data = file->data;
	T = t_taglist_new();

	for (i = 0; i < countof(taglibkeys); i++) {
		if (key != NULL) {
			if (strcasecmp(key, taglibkeys[i]) != 0)
				continue;
		}
		value = NULL;
		switch (i) {
		case 0:
			value = taglib_tag_album(data->tag);
			break;
		case 1:
			value = taglib_tag_artist(data->tag);
			break;
		case 2:
			value = taglib_tag_comment(data->tag);
			break;
		case 3:
			uintval = taglib_tag_year(data->tag);
			if (uintval > 0)
				(void)xasprintf(&value, "%04u", uintval);
			break;
		case 4:
			value = taglib_tag_genre(data->tag);
			break;
		case 5:
			value = taglib_tag_title(data->tag);
			break;
		case 6:
			uintval = taglib_tag_track(data->tag);
			if (uintval > 0)
				(void)xasprintf(&value, "%02u", uintval);
			break;
		}
		if (value && t_strempty(value))
			/* clean value, when TagLib return "" we return NULL */
			freex(value);

		if (value != NULL) {
			t_taglist_insert(T, taglibkeys[i], value);
			freex(value);
		}
		if (key != NULL)
			break;
	}

	return (T);
}


static bool
t_file_clear(struct t_file *restrict file, const struct t_taglist *restrict T)
{
	int	i;
	struct t_generic_data *data;

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);
	t_error_clear(file);

	data = file->data;

	for (i = 0; i < countof(taglibkeys); i++) {
		if (T == NULL || t_taglist_filter_count(T, taglibkeys[i], T_TAG_FIRST)) {
			switch (i) {
			case 0:
				taglib_tag_set_album(data->tag, "");
				break;
			case 1:
				taglib_tag_set_artist(data->tag, "");
				break;
			case 2:
				taglib_tag_set_comment(data->tag, "");
				break;
			case 3:
				taglib_tag_set_year(data->tag, 0);
				break;
			case 4:
				taglib_tag_set_genre(data->tag, "");
				break;
			case 5:
				taglib_tag_set_title(data->tag, "");
				break;
			case 6:
				taglib_tag_set_track(data->tag, 0);
				break;
			}
			file->dirty++;
		}
	}

	return (true);
}


static bool
t_file_add(struct t_file *restrict file, const struct t_taglist *restrict T)
{
	bool	isstrf;
	struct t_generic_data *data;
	struct t_tag *t;
	void (*strf)(TagLib_Tag *, const char *);
	void (*uif)(TagLib_Tag *, unsigned int);

	assert_not_null(file);
	assert_not_null(file->data);
	assert(file->libid == libid);
	assert_not_null(T);
	t_error_clear(file);

	data = file->data;

	t_tagQ_foreach(t, T->tags) {
		/* detect key function to use */
		isstrf = true;
		assert_not_null(t->key);
		if (strcmp(t->key, "artist") == 0)
			strf = taglib_tag_set_artist;
		else if (strcmp(t->key, "album") == 0)
			strf = taglib_tag_set_album;
		else if (strcmp(t->key, "description") == 0)
			strf = taglib_tag_set_comment;
		else if (strcmp(t->key, "genre") == 0)
			strf = taglib_tag_set_genre;
		else if (strcmp(t->key, "title") == 0)
			strf = taglib_tag_set_title;
		else {
			isstrf = false;
			if (strcmp(t->key, "tracknumber") == 0)
				uif = taglib_tag_set_track;
			else if (strcmp(t->key, "date") == 0)
				uif = taglib_tag_set_year;
			else {
				t_error_set(file,
				    "%s backend can't handle `%s' tag", file->libid, t->key);
				return (false);
			}
		}
		if (isstrf)
			strf(data->tag, t->value);
		else {
			char	*endptr;
			unsigned long ulongval;
			ulongval = strtoul(t->value, &endptr, 10);
			if (endptr == t->value || *endptr != '\0') {
				t_error_set(file, "invalid unsigned int argument for %s: `%s'",
				    t->key, t->value);
				return (false);
			} else if (ulongval > UINT_MAX) {
				t_error_set(file, "invalid unsigned int argument for %s: `%s' (too large)",
				    t->key, t->value);
				return (false);
			} else if (errno) {
				/* should be EINVAL (ERANGE catched by last condition). */
				assert_fail();
			}
			uif(data->tag, (unsigned int)ulongval);
		}
		file->dirty++;
	}

	return (true);
}

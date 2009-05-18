/*
 * t_ftgeneric.c
 *
 * a generic tagutil backend, using TagLib.
 */
#include "t_config.h"

#include <tag_c.h>

#include "t_file.h"
#include "t_ftgeneric.h"
#include "t_toolkit.h"


struct ftgeneric_data {
    TagLib_File *file;
    TagLib_Tag  *tag;
    char *year, *track;
};

__t__nonnull(1)
int ftgeneric_destroy(struct tfile *restrict self);
__t__nonnull(1)
int ftgeneric_save(struct tfile *restrict self);

__t__nonnull(1) __t__nonnull(2)
const char * ftgeneric_get(const struct tfile *restrict self,
        const char *restrict key);
__t__nonnull(1) __t__nonnull(2) __t__nonnull(3)
int ftgeneric_set(struct tfile *restrict self, const char *restrict key,
        const char *restrict newval);

__t__nonnull(1)
int ftgeneric_tagcount(const struct tfile *restrict self);
__t__nonnull(1)
const char ** ftgeneric_tagkeys(const struct tfile *restrict self);


int
ftgeneric_destroy(struct tfile *restrict self)
{
    struct ftgeneric_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;
    free(d->track);
    free(d->year);
    taglib_tag_free_strings(); /* XXX: thread-safe? */
    taglib_file_free(d->file);

    xfree(self);
    return (true);
}


int
ftgeneric_save(struct tfile *restrict self)
{
    struct ftgeneric_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;

    if (taglib_file_save(d->file))
		return (0);
	else
		return (1);
}


const char *
ftgeneric_get(const struct tfile *restrict self, const char *restrict key)
{
    struct ftgeneric_data *d;
    const char *ret;

    assert_not_null(self);
    assert_not_null(self->data);
    assert_not_null(key);

    d = self->data;

    if (strcmp(key, "artist") == 0)
        ret = taglib_tag_artist(d->tag);
    else if (strcmp(key, "album") == 0)
        ret = taglib_tag_album(d->tag);
    else if (strcmp(key, "comment") == 0)
        ret = taglib_tag_comment(d->tag);
    else if (strcmp(key, "genre") == 0)
        ret = taglib_tag_genre(d->tag);
    else if (strcmp(key, "title") == 0)
        ret = taglib_tag_title(d->tag);
    else if (strcmp(key, "track") == 0) {
        if (d->track == NULL)
            (void)xasprintf(&d->track, "%u", taglib_tag_track(d->tag));
        ret = d->track;
    }
    else if (strcmp(key, "year") == 0) {
        if (d->year == NULL)
            (void)xasprintf(&d->year, "%u", taglib_tag_year(d->tag));
        ret = d->year;
    }
    else {
        warnx("taglib: can't handle '%s' tag", key);
        ret = NULL;
    }

    return (ret);
}


int
ftgeneric_set(struct tfile *restrict self, const char *restrict key,
        const char *restrict newval)
{
    struct ftgeneric_data *d;
    int ret = 0;
    unsigned int uintval;
    char *endptr;

    assert_not_null(self);
    assert_not_null(self->data);
    assert_not_null(key);
    assert_not_null(newval);

    d = self->data;

    if (strcmp(key, "artist") == 0)
        taglib_tag_set_artist(d->tag, newval);
    else if (strcmp(key, "album") == 0)
        taglib_tag_set_album(d->tag, newval);
    else if (strcmp(key, "comment") == 0)
        taglib_tag_set_comment(d->tag, newval);
    else if (strcmp(key, "genre") == 0)
        taglib_tag_set_genre(d->tag, newval);
    else if (strcmp(key, "title") == 0)
        taglib_tag_set_title(d->tag, newval);
    else if (strcmp(key, "track") == 0) {
        uintval = strtoul(newval, &endptr, 10);
        if (endptr == newval || *endptr != '\0') {
            warnx("set: Invalid track argument: '%s'", newval);
            ret = 1;
        }
        else {
            free(d->track);
            d->track = NULL;
            taglib_tag_set_track(d->tag, uintval);
        }
    }
    else if (strcmp(key, "year") == 0) {
        uintval = strtoul(newval, &endptr, 10);
        if (endptr == newval || *endptr != '\0') {
            warnx("set: Invalid year argument: '%s'", newval);
            ret = 1;
        }
        else {
            free(d->year);
            d->year = NULL;
            taglib_tag_set_year(d->tag, uintval);
        }
    }
    else {
        warnx("taglib: can't handle '%s' tag", key);
        ret = 2;
    }

    return (ret);
}


int
ftgeneric_tagcount(const struct tfile *restrict self)
{

    assert_not_null(self);
    assert_not_null(self->data);

    return (7);
}


const char **
ftgeneric_tagkeys(const struct tfile *restrict self)
{
    static const char *tagkeys[] = {
        "title", "artist", "album", "track", "year", "genre", "comment",
    };

    assert_not_null(self);
    return (tagkeys);
}


struct tfile *
ftgeneric_new(const char *restrict path)
{
    TagLib_File *f;
    struct tfile *ret;
    size_t size;
    char *s;
    struct ftgeneric_data *d;

    assert_not_null(path);

    /* taglib specific init */
    taglib_set_strings_unicode(has_match(getenv("LC_ALL"), "utf-?8"));
    taglib_set_string_management_enabled(true);

    f = taglib_file_new(path);
    if (f == NULL || !taglib_file_is_valid(f))
        return (NULL);

    size = (strlen(path) + 1) * sizeof(char);
    ret = xmalloc(sizeof(struct tfile) + sizeof(struct ftgeneric_data) + size);

    d = (struct ftgeneric_data *)(ret + 1);
    d->file  = f;
    d->tag   = taglib_file_tag(f);
    d->year  = NULL;
    d->track = NULL;
    ret->data = d;

    s = (char *)(d + 1);
    strlcpy(s, path, size);
    ret->path = s;

    ret->create   = ftgeneric_new;
    ret->save     = ftgeneric_save;
    ret->destroy  = ftgeneric_destroy;
    ret->get      = ftgeneric_get;
    ret->set      = ftgeneric_set;
    ret->tagcount = ftgeneric_tagcount;
    ret->tagkeys  = ftgeneric_tagkeys;

    return (ret);
}


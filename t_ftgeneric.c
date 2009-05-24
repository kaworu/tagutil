/*
 * t_ftgeneric.c
 *
 * a generic tagutil backend, using TagLib.
 */
#include "t_config.h"

/* TagLib headers */
#include <tag_c.h>

#include <stdbool.h>

#include "t_file.h"
#include "t_ftgeneric.h"
#include "t_toolkit.h"


struct ftgeneric_data {
    TagLib_File *file;
    TagLib_Tag  *tag;
};

_t__nonnull(1)
void ftgeneric_destroy(struct tfile *restrict self);
_t__nonnull(1)
bool ftgeneric_save(struct tfile *restrict self);

_t__nonnull(1) _t__nonnull(2)
char * ftgeneric_get(const struct tfile *restrict self,
        const char *restrict key);
_t__nonnull(1) _t__nonnull(2) _t__nonnull(3)
enum tfile_set_status ftgeneric_set(struct tfile *restrict self,
        const char *restrict key, const char *restrict newval);

_t__nonnull(1)
long ftgeneric_tagcount(const struct tfile *restrict self);
_t__nonnull(1)
char ** ftgeneric_tagkeys(const struct tfile *restrict self);


void
ftgeneric_destroy(struct tfile *restrict self)
{
    struct ftgeneric_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;
    taglib_file_free(d->file);

    xfree(self);
}


bool
ftgeneric_save(struct tfile *restrict self)
{
    struct ftgeneric_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;

    if (taglib_file_save(d->file))
		return (true);
	else
		return (false);
}


char *
ftgeneric_get(const struct tfile *restrict self, const char *restrict key)
{
    struct ftgeneric_data *d;
    char *ret;

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
    else if (strcmp(key, "track") == 0)
        (void)xasprintf(&ret, "%02u", taglib_tag_track(d->tag));
    else if (strcmp(key, "year") == 0)
        (void)xasprintf(&ret, "%04u", taglib_tag_year(d->tag));
    else {
        warnx("ftgeneric_get: %s backend can't handle `%s' tag", self->lib, key);
        ret = NULL;
    }

    return (ret);
}


enum tfile_set_status
ftgeneric_set(struct tfile *restrict self, const char *restrict key,
        const char *restrict newval)
{
    bool erase = false;
    struct ftgeneric_data *d;
    unsigned int uintval;
    char *endptr;

    assert_not_null(self);
    assert_not_null(self->data);
    assert_not_null(key);

    d = self->data;
    /* this backend can't destroy tags */
    if (newval == NULL) {
        erase = true;
        newval = "";
    }

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
        if (!erase && (endptr == newval || *endptr != '\0')) {
            warnx("ftgeneric_set: need Int track argument, got: `%s'", newval);
            return (TFILE_SET_STATUS_BADARG);
        }
        else
            taglib_tag_set_track(d->tag, uintval);
    }
    else if (strcmp(key, "year") == 0) {
        uintval = strtoul(newval, &endptr, 10);
        if (!erase && (endptr == newval || *endptr != '\0')) {
            warnx("ftgeneric_set: need Int year argument, got: `%s'", newval);
            return (TFILE_SET_STATUS_BADARG);
        }
        else
            taglib_tag_set_year(d->tag, uintval);
    }
    else {
        warnx("ft_generic_set: %s backend can't handle `%s' tag", self->lib, key);
        return (TFILE_SET_STATUS_LIBERROR);
    }

    return (TFILE_SET_STATUS_OK);
}


long
ftgeneric_tagcount(const struct tfile *restrict self)
{

    assert_not_null(self);
    assert_not_null(self->data);

    return (7);
}


char **
ftgeneric_tagkeys(const struct tfile *restrict self)
{
    char **ary;

    assert_not_null(self);

    ary = xmalloc(7 * sizeof(char *));
    ary[0] = xstrdup("album");
    ary[1] = xstrdup("artist");
    ary[2] = xstrdup("comment");
    ary[3] = xstrdup("genre");
    ary[4] = xstrdup("title");
    ary[5] = xstrdup("track");
    ary[6] = xstrdup("year");

    return (ary);
}


void
ftgeneric_init(void)
{

    /* TagLib specific init */
    taglib_set_strings_unicode(has_match(getenv("LC_ALL"), "utf-?8"));
    taglib_set_string_management_enabled(false);
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

    f = taglib_file_new(path);
    if (f == NULL || !taglib_file_is_valid(f))
        return (NULL);

    size = (strlen(path) + 1) * sizeof(char);
    ret = xmalloc(sizeof(struct tfile) + sizeof(struct ftgeneric_data) + size);

    d = (struct ftgeneric_data *)(ret + 1);
    d->file  = f;
    d->tag   = taglib_file_tag(f);
    ret->data = d;

    s = (char *)(d + 1);
    (void)strlcpy(s, path, size);
    ret->path = s;

    ret->create   = ftgeneric_new;
    ret->save     = ftgeneric_save;
    ret->destroy  = ftgeneric_destroy;
    ret->get      = ftgeneric_get;
    ret->set      = ftgeneric_set;
    ret->tagcount = ftgeneric_tagcount;
    ret->tagkeys  = ftgeneric_tagkeys;

    ret->lib = "TagLib";
    return (ret);
}


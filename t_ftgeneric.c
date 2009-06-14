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

_t__nonnull(1)
struct tag_list * ftgeneric_get(struct tfile *restrict self,
        const char *restrict key);
_t__nonnull(1)
bool ftgeneric_clear(struct tfile *restrict self, const struct tag_list *restrict T);
_t__nonnull(1) _t__nonnull(2)
bool ftgeneric_add(struct tfile *restrict self, const struct tag_list *restrict T);


void
ftgeneric_destroy(struct tfile *restrict self)
{
    struct ftgeneric_data *d;

    assert_not_null(self);
    assert_not_null(self->data);

    d = self->data;
    taglib_file_free(d->file);
    reset_error_msg(self);
    freex(self);
}


bool
ftgeneric_save(struct tfile *restrict self)
{
    bool ok;
    struct ftgeneric_data *d;

    assert_not_null(self);
    assert_not_null(self->data);
    reset_error_msg(self);

    d = self->data;
	ok = taglib_file_save(d->file);
    if (!ok)
        set_error_msg(self, "%s error", self->lib);
    return (ok);
}


static const char * _taglibkeys[] = {
    "album", "artist", "comment", "date", "genre", "title", "tracknumber"
};
struct tag_list *
ftgeneric_get(struct tfile *restrict self, const char *restrict key)
{
    int i;
    unsigned int uintval;
    struct ftgeneric_data *d;
    struct tag_list *T;
    char *value;

    assert_not_null(self);
    assert_not_null(self->data);
    reset_error_msg(self);

    d = self->data;
    T = new_tag_list();

    for (i = 0; i < 7; i++) {
        if (key) {
            if (strcasecmp(key, _taglibkeys[i]) != 0)
                continue;
        }
        value = NULL;
        switch (i) {
        case 0:
            value = taglib_tag_album(d->tag);
            break;
        case 1:
            value = taglib_tag_artist(d->tag);
            break;
        case 2:
            value = taglib_tag_comment(d->tag);
            break;
        case 3:
            uintval = taglib_tag_year(d->tag);
            if (uintval > 0)
                (void)xasprintf(&value, "%04u", taglib_tag_year(d->tag));
            break;
        case 4:
            value = taglib_tag_genre(d->tag);
            break;
        case 5:
            value = taglib_tag_title(d->tag);
            break;
        case 6:
            uintval = taglib_tag_track(d->tag);
            if (uintval > 0)
                (void)xasprintf(&value, "%02u", uintval);
            break;
        }
        if (value && strempty(value))
        /* clean value, when TagLib return "" we return NULL */
            freex(value);

        if (value) {
            tag_list_insert(T, _taglibkeys[i], value);
            freex(value);
        }
        if (key)
            break;
    }

    return (T);
}


bool
ftgeneric_clear(struct tfile *restrict self, const struct tag_list *restrict T)
{
    int i;
    struct ftgeneric_data *d;

    assert_not_null(self);
    assert_not_null(self->data);
    reset_error_msg(self);

    d = self->data;

    for (i = 0; i < 7; i++) {
        if (T && tag_list_search(T, _taglibkeys[i]) == NULL)
            continue;
        switch (i) {
        case 0:
            taglib_tag_set_album(d->tag, "");
            break;
        case 1:
            taglib_tag_set_artist(d->tag, "");
            break;
        case 2:
            taglib_tag_set_comment(d->tag, "");
            break;
        case 3:
            taglib_tag_set_year(d->tag, 0);
            break;
        case 4:
            taglib_tag_set_genre(d->tag, "");
            break;
        case 5:
            taglib_tag_set_title(d->tag, "");
            break;
        case 6:
            taglib_tag_set_track(d->tag, 0);
            break;
        }
    }

    return (true);
}


bool
ftgeneric_add(struct tfile *restrict self, const struct tag_list *restrict T)
{
    struct ftgeneric_data *d;
    unsigned int uintval;
    char *endptr;
    struct ttag  *t;
    struct ttagv *v;
    bool strfunc;
    void (*strf)(TagLib_Tag *, const char *);
    void (*uif)(TagLib_Tag *, unsigned int);

    assert_not_null(T);
    assert_not_null(self);
    assert_not_null(self->data);
    reset_error_msg(self);

    d = self->data;

    TAILQ_FOREACH(t, T->tags, next) {
        /* detect key function to use */
        strfunc = true;
        assert_not_null(t->key);
        if (strcmp(t->key, "artist") == 0)
            strf = taglib_tag_set_artist;
        else if (strcmp(t->key, "album") == 0)
            strf = taglib_tag_set_album;
        else if (strcmp(t->key, "comment") == 0)
            strf = taglib_tag_set_comment;
        else if (strcmp(t->key, "genre") == 0)
            strf = taglib_tag_set_genre;
        else if (strcmp(t->key, "title") == 0)
            strf = taglib_tag_set_title;
        else {
            strfunc = false;
            if (strcmp(t->key, "tracknumber") == 0)
                uif = taglib_tag_set_track;
            else if (strcmp(t->key, "date") == 0)
                uif = taglib_tag_set_year;
            else {
                set_error_msg(self,
                        "%s backend can't handle `%s' tag", self->lib, t->key);
                return (false);
            }
        }

        if (t->vcount != 1) {
            set_error_msg(self,
                    "%s backend can only set one tag %s, got %zd",
                    self->lib, t->key, t->vcount);
            return (false);
        }
        v = TAILQ_FIRST(t->values);
        assert_not_null(v->value);
        if (strfunc)
            strf(d->tag, v->value);
        else {
            uintval = (unsigned int)strtoul(v->value, &endptr, 10);
            if (endptr == v->value || *endptr != '\0') {
                set_error_msg(self, "need Int argument for %s, got: `%s'",
                        t->key, v->value);
                return (false);
            }
            uif(d->tag, uintval);
        }
    }

    return (true);
}


void
ftgeneric_init(void)
{
    char *lcall, *dot;

    /* TagLib specific init */
    lcall = getenv("LC_ALL");
    dot = strchr(lcall, '.');
    if (dot && strcmp(dot + 1, "UTF-8"))
        taglib_set_strings_unicode(true);

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
    ret->clear    = ftgeneric_clear;
    ret->add      = ftgeneric_add;

    ret->lib = "TagLib";
    ret->errmsg = NULL;
    return (ret);
}


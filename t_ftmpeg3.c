/*
 * t_ftmpeg3.c
 *
 * MPEG layer 3 format handler with ID3Lib.
 */

/*
 * XXX:
 * HACK: id3/globals.h will define bool, true and false ifndef __cplusplus. !!!
 *          so we include "id3.h" before anything, undef false/true and include
 *          <stdbool.h>
 */

/* ID3Lib headers */
#include "id3.h"
#undef true
#undef false

#include <stdbool.h>
#include <string.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_file.h"
#include "t_ftmpeg3.h"


struct t_ftmpeg3_data {
    /* TODO */
};


_t__nonnull(1)
void t_ftmpeg3_destroy(struct t_file *restrict file);

_t__nonnull(1)
bool t_ftmpeg3_save(struct t_file *restrict file);

_t__nonnull(1)
struct t_taglist * t_ftmpeg3_get(struct t_file *restrict file,
        const char *restrict key);

_t__nonnull(1)
bool t_ftmpeg3_clear(struct t_file *restrict file,
        const struct t_taglist *restrict T);

_t__nonnull(1) _t__nonnull(2)
bool t_ftmpeg3_add(struct t_file *restrict file,
        const struct t_taglist *restrict T);


void
t_ftmpeg3_destroy(struct t_file *restrict file)
{
    struct t_ftmpeg3_data *d;

    assert_not_null(file);
    assert_not_null(file->data);

    d = file->data;

    /* TODO */
    t_error_clear(file);
    freex(file);
}


bool
t_ftmpeg3_save(struct t_file *restrict file)
{
    struct t_ftmpeg3_data *d;

    assert_not_null(file);
    assert_not_null(file->data);
    t_error_clear(file);

    d = file->data;

    /* TODO */
    t_error_set(file, "t_ftmpeg3_save: still need to be implemented.");
    return (false);
}


struct t_taglist *
t_ftmpeg3_get(struct t_file *restrict file, const char *restrict key)
{
    struct t_taglist *T;
    struct t_ftmpeg3_data *d;

    assert_not_null(file);
    assert_not_null(file->data);
    t_error_clear(file);

    d = file->data;
    T = t_taglist_new();

    /* TODO */
    t_error_set(file, "t_ftmpeg3_get: still need to be implemented.");
    return (NULL);
}


bool
t_ftmpeg3_clear(struct t_file *restrict file, const struct t_taglist *restrict T)
{
    struct t_ftmpeg3_data *d;

    assert_not_null(file);
    assert_not_null(file->data);
    t_error_clear(file);

    d = file->data;

    /* TODO */
    t_error_set(file, "t_ftmpeg3_clear: still need to be implemented.");
    return (false);
}


bool
t_ftmpeg3_add(struct t_file *restrict file, const struct t_taglist *restrict T)
{
    struct t_ftmpeg3_data *d;

    assert_not_null(file);
    assert_not_null(file->data);
    assert_not_null(T);
    t_error_clear(file);

    d = file->data;

    /* TODO */
    t_error_set(file, "t_ftmpeg3_add: still need to be implemented.");
    return (false);
}


void
t_ftmpeg3_init(void)
{
    return;
}


struct t_file *
t_ftmpeg3_new(const char *restrict path)
{
    char *s;
    size_t size;
    struct t_file *ret;
    struct t_ftmpeg3_data *d;

    assert_not_null(path);

    /* TODO: mpeg3 init */
    return (NULL);
    size = strlen(path) + 1;
    ret = xmalloc(sizeof(struct t_file) + sizeof(struct t_ftmpeg3_data) + size);

    d = (struct t_ftmpeg3_data *)(ret + 1);
    /* TODO: fill d */
    ret->data = d;

    s = (char *)(d + 1);
    assert(strlcpy(s, path, size) < size);
    ret->path = s;

    ret->create   = t_ftmpeg3_new;
    ret->save     = t_ftmpeg3_save;
    ret->destroy  = t_ftmpeg3_destroy;
    ret->get      = t_ftmpeg3_get;
    ret->clear    = t_ftmpeg3_clear;
    ret->add      = t_ftmpeg3_add;

    ret->lib = "ID3Lib";
    t_error_init(ret);
    return (ret);
}


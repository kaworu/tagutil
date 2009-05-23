/*
 * t_yaml.c
 *
 * yaml tagutil interface, using libyaml.
 */
#include "t_config.h"

#include <sys/param.h>

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/* libyaml headers */
#include <yaml.h>

#include "t_toolkit.h"
#include "t_file.h"
#include "t_yaml.h"


struct whdl_data {
    char *str;
    size_t alloclen, len;
};

int yaml_write_handler(void *dataptr, unsigned char *buffer, size_t size);


char *
tags_to_yaml(const struct tfile *restrict file)
{
    yaml_emitter_t emitter;
    yaml_event_t event;
    struct whdl_data *data;
    char *ret;
    char **tagkeys = NULL, *val;
    int count = 0, i;

    data = xmalloc(sizeof(struct whdl_data));
    (void)xasprintf(&data->str, "# %s\n", file->path);
    data->len = strlen(data->str);
    data->alloclen = data->len + 1;

    /* Create the Emitter object. */
    if (!yaml_emitter_initialize(&emitter))
        goto emitter_error;

    yaml_emitter_set_output(&emitter, yaml_write_handler, data);
    yaml_emitter_set_unicode(&emitter, 1);

    /* Create and emit the STREAM-START event. */
    if (!yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING))
        goto event_error;
    if (!yaml_emitter_emit(&emitter, &event))
        goto emitter_error;

    /* Create and emit the DOCUMENT-START event. */
    if (!yaml_document_start_event_initialize(&event, NULL, NULL, NULL,
                /* implicit */0))
        goto event_error;
    if (!yaml_emitter_emit(&emitter, &event))
        goto emitter_error;

    /* Create and emit the MAPPING-START event. */
    if (!yaml_mapping_start_event_initialize(&event, NULL,
                (yaml_char_t *)YAML_MAP_TAG, 1, YAML_BLOCK_MAPPING_STYLE))
        goto event_error;
    if (!yaml_emitter_emit(&emitter, &event))
        goto emitter_error;

    count = file->tagcount(file);
    tagkeys = file->tagkeys(file);
    if (tagkeys == NULL)
        errx(-1, "tags_to_yaml: NULL tagkeys (%s backend)", file->lib);
    for (i = 0; i < count; i++) {
        /* emit the key */
        if (!yaml_scalar_event_initialize(&event, NULL,
                    (yaml_char_t *)YAML_STR_TAG, (yaml_char_t *)tagkeys[i], -1,
                    1, 1, YAML_PLAIN_SCALAR_STYLE))
            goto event_error;
        if (!yaml_emitter_emit(&emitter, &event))
            goto emitter_error;
        /* emit the value */
        val = file->get(file, tagkeys[i]);
        if (val == NULL)
            errx(-1, "tags_to_yaml: bad tagkeys/get (%s backend)", file->lib);
        if (!yaml_scalar_event_initialize(&event, NULL,
                    (yaml_char_t *)YAML_STR_TAG, (yaml_char_t *)val, -1, 1, 1,
                    YAML_PLAIN_SCALAR_STYLE)) {
            xfree(val);
            goto event_error;
        }
        if (!yaml_emitter_emit(&emitter, &event)) {
            xfree(val);
            goto emitter_error;
        }
        xfree(tagkeys[i]);
        xfree(val);
    }
    xfree(tagkeys);

    /* Create and emit the MAPPING-END event. */
    if (!yaml_mapping_end_event_initialize(&event))
        goto event_error;
    if (!yaml_emitter_emit(&emitter, &event))
        goto emitter_error;

    /* Create and emit the DOCUMENT-END event. */
    if (!yaml_document_end_event_initialize(&event, /* implicit */1))
        goto event_error;
    if (!yaml_emitter_emit(&emitter, &event))
        goto emitter_error;

    /* Create and emit the STREAM-END event. */
    if (!yaml_stream_end_event_initialize(&event))
        goto event_error;
    if (!yaml_emitter_emit(&emitter, &event))
        goto emitter_error;

    /* Destroy the Emitter object. */
    yaml_emitter_delete(&emitter);
    yaml_event_delete(&event);
    ret = data->str;
    xfree(data);
    return (ret);

    /* On error. */

event_error:
    errx(ENOMEM, "tags_to_yaml: can't init event");
    /* NOTREACHED */

emitter_error:
    warnx("tags_to_yaml: emit error");
    /* Destroy the Emitter object. */
    yaml_emitter_delete(&emitter);
    yaml_event_delete(&event);
    if (tagkeys != NULL) {
        for (i = 0; i < count; i++)
            free(tagkeys[i]);
        xfree(tagkeys);
    }
    xfree(data->str);
    xfree(data);
    return (NULL);
}


bool
yaml_to_tags(struct tfile *restrict file, FILE *restrict stream)
{
    return (true);
}


int
yaml_write_handler(void *dataptr, unsigned char *buffer, size_t size)
{
    bool error = false;
    struct whdl_data *data;

    if (dataptr == NULL || buffer == NULL)
        error = true;
    else {
        data = dataptr;
        if (data->len + size + 1 > data->alloclen) {
            data->alloclen += size + BUFSIZ;
            data->str = xrealloc(data->str, data->alloclen);
        }
        memcpy(data->str + data->len, buffer, size);
        data->len += size;
        data->str[data->len] = '\0';
    }

    if (error)
        return (0);
    else
        return (1);
}


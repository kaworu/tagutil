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
#include "t_setter.h"
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
    char **tagkeys = NULL, *value;
    int count = 0, i;

    assert_not_null(file);

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
        value = file->get(file, tagkeys[i]);
        if (value == NULL)
            errx(-1, "tags_to_yaml: bad tagkeys/get (%s backend)", file->lib);
        if (!yaml_scalar_event_initialize(&event, NULL,
                    (yaml_char_t *)YAML_STR_TAG, (yaml_char_t *)value, -1, 1, 1,
                    YAML_PLAIN_SCALAR_STYLE)) {
            xfree(value);
            goto event_error;
        }
        if (!yaml_emitter_emit(&emitter, &event)) {
            xfree(value);
            goto emitter_error;
        }
        xfree(tagkeys[i]);
        xfree(value);
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
    errx(-1, "tags_to_yaml: emit error");
}


bool
yaml_to_tags(struct tfile *restrict file, FILE *restrict stream)
{
    struct setter_q *Q;
    struct setter_item *item;
    yaml_parser_t parser;
    yaml_event_t event;
    bool stop = false;
    bool inmap = false, donemap = false;
    bool ret = true;
    char *key = NULL, *value = NULL;

    assert_not_null(file);
    assert_not_null(stream);

    if (!yaml_parser_initialize(&parser))
        goto parser_error;

    yaml_parser_set_input_file(&parser, stream);

    Q = new_setter();
    while (!stop) {
        if (!yaml_parser_parse(&parser, &event))
            goto parser_error;
        switch (event.type) {
        case YAML_DOCUMENT_END_EVENT:   /* FALLTHROUGH */
        case YAML_STREAM_START_EVENT:   /* FALLTHROUGH */
        case YAML_DOCUMENT_START_EVENT: /* FALLTHROUGH */
        case YAML_NO_EVENT:
            continue;

        case YAML_ALIAS_EVENT:
            warnx("YAML parser got unexpected YAML_ALIAS_EVENT");
            stop = true;
            ret = false;
            break;
        case YAML_SEQUENCE_START_EVENT:
            warnx("YAML parser got unexpected YAML_SEQUENCE_START_EVENT");
            stop = true;
            ret = false;
            break;
        case YAML_SEQUENCE_END_EVENT:
            warnx("YAML parser got unexpected YAML_SEQUENCE_END_EVENT");
            stop = true;
            ret = false;
            break;

        case YAML_MAPPING_START_EVENT:
            if (inmap) {
                warnx("unexpected nested mapping");
                stop = true;
                ret = false;
            }
            else if (donemap) {
                warnx("unexpected extra mapping, needed only one");
                stop = true;
                ret = false;
            }
            else
                inmap = true;
            break;
        case YAML_MAPPING_END_EVENT:
            donemap = true;
            inmap = false;
            break;
        case YAML_SCALAR_EVENT:
            if (!inmap) {
                warnx("unexpected scalar");
                stop = true;
                ret = false;
            }
            else if (donemap) {
                warnx("unexpected extra scalar after mapping");
                stop = true;
                ret = false;
            }
            else if (key == NULL) {
                key = xcalloc(event.data.scalar.length + 1, sizeof(char));
                memcpy(key, event.data.scalar.value, event.data.scalar.length);
            }
            else {
                value = xcalloc(event.data.scalar.length + 1, sizeof(char));
                memcpy(value, event.data.scalar.value,
                        event.data.scalar.length);
                (void)setter_add2(Q, key, value);
                xfree(key);
                xfree(value);
            }
            break;
        case YAML_STREAM_END_EVENT:
            stop = true;
            break;
        }
    }

    /* destroy non-present tags! TODO */
    STAILQ_FOREACH(item, Q, next) {
        (void)file->set(file, item->key, item->value);
    }

    destroy_setter(Q);
    yaml_event_delete(&event);
    yaml_parser_delete(&parser);
    return (ret);

parser_error:
    switch (parser.error) {
    case YAML_MEMORY_ERROR:
        err(ENOMEM, "YAML Parser");
        /* NOTREACHED */
    default:
        err(-1, "parser error, still TODO :)"); /* TODO */
        /* NOTREACHED */
    }
    yaml_event_delete(&event);
    yaml_parser_delete(&parser);
    return (false);
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


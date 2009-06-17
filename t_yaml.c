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


int yaml_write_handler(void *data, unsigned char *buffer, size_t size);


char *
tags_to_yaml(struct tfile *restrict file)
{
    yaml_emitter_t emitter;
    yaml_event_t event;
    struct strbuf *sb;
    char *head, *ret;
    size_t headlen;
    struct tag_list *T;
    struct ttag  *t;
    struct ttagv *v;

    assert_not_null(file);

    headlen = xasprintf(&head, "# %s\n", file->path);
    sb = new_strbuf();
    strbuf_add(sb, head, headlen);

    /* Create the Emitter object. */
    if (!yaml_emitter_initialize(&emitter))
        goto emitter_error;

    yaml_emitter_set_output(&emitter, yaml_write_handler, sb);
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

    T = file->get(file, NULL);
    if (T == NULL) {
        destroy_strbuf(sb);
        return (NULL);
    }
    TAILQ_FOREACH(t, T->tags, next) {
        TAILQ_FOREACH(v, t->values, next) {
            /* emit the key */
            if (!yaml_scalar_event_initialize(&event, NULL,
                        (yaml_char_t *)YAML_STR_TAG, (yaml_char_t *)t->key, -1,
                        1, 1, YAML_PLAIN_SCALAR_STYLE))
            goto event_error;
            if (!yaml_emitter_emit(&emitter, &event))
                goto emitter_error;
            /* emit the value */
        if (!yaml_scalar_event_initialize(&event, NULL,
                    (yaml_char_t *)YAML_STR_TAG, (yaml_char_t *)v->value, -1,
                    1, 1, YAML_PLAIN_SCALAR_STYLE)) {
            goto event_error;
        }
        if (!yaml_emitter_emit(&emitter, &event))
            goto emitter_error;
        }
    }
    destroy_tag_list(T);

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
    ret = strbuf_get(sb);
    destroy_strbuf(sb);
    return (ret);

event_error:
    errx(errno = ENOMEM, "tags_to_yaml: can't init event");
    /* NOTREACHED */
emitter_error:
    errx(-1, "tags_to_yaml: emit error");
}


struct tag_list *
yaml_to_tags(struct tfile *restrict file, FILE *restrict stream)
{
    struct tag_list *T;
    yaml_parser_t parser;
    yaml_event_t event;
    bool stop = false;
    bool inmap = false, donemap = false;
    char *key = NULL, *value = NULL;

    assert_not_null(stream);
    assert_not_null(file);

    T = new_tag_list();

    if (!yaml_parser_initialize(&parser))
        goto parser_error;
    yaml_parser_set_input_file(&parser, stream);

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
            t_error_set(T, "YAML parser got unexpected "
                    "YAML_ALIAS_EVENT at line %zu",
                   parser.context_mark.line + 1);
            stop = true;
            break;
        case YAML_SEQUENCE_START_EVENT:
            t_error_set(T, "YAML parser got unexpected "
                    "YAML_SEQUENCE_START_EVENT at line %zu",
                    parser.context_mark.line + 1);
            stop = true;
            break;
        case YAML_SEQUENCE_END_EVENT:
            t_error_set(T, "YAML parser got unexpected "
                    "YAML_SEQUENCE_END_EVENT at line %zu",
                    parser.context_mark.line + 1);
            stop = true;
            break;

        case YAML_MAPPING_START_EVENT:
            if (inmap) {
                t_error_set(T, "unexpected nested mapping at line %zu",
                        parser.context_mark.line + 1);
                stop = true;
            }
            else if (donemap) {
                t_error_set(T, "unexpected extra mapping, needed only one. "
                        "at line %zu", parser.context_mark.line + 1);
                stop = true;
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
                t_error_set(T, "unexpected scalar at line %zu",
                        parser.context_mark.line + 1);
                stop = true;
            }
            else if (donemap) {
                t_error_set(T, "unexpected extra scalar after mapping at line %zu",
                        parser.context_mark.line + 1);
                stop = true;
            }
            else if (key == NULL) {
                key = xcalloc(event.data.scalar.length + 1, sizeof(char));
                memcpy(key, event.data.scalar.value, event.data.scalar.length);
            }
            else {
                value = xcalloc(event.data.scalar.length + 1, sizeof(char));
                memcpy(value, event.data.scalar.value,
                        event.data.scalar.length);
                tag_list_insert(T, key, value);
                freex(key);
                freex(value);
            }
            break;
        case YAML_STREAM_END_EVENT:
            stop = true;
            break;
        }
    }

    yaml_event_delete(&event);
    yaml_parser_delete(&parser);
    return (T);

parser_error:
    switch (parser.error) {
    case YAML_MEMORY_ERROR:
        t_error_set(T, "yaml_to_tags: YAML Parser (ENOMEM)");
        break;
    case YAML_READER_ERROR:
        if (parser.problem_value != -1) {
            t_error_set(T, "yaml_to_tags: Reader error: %s: #%X at %zu\n",
                    parser.problem,
                    parser.problem_value,
                    parser.problem_offset);
        }
        else {
            t_error_set(T, "yaml_to_tags: Reader error: %s at %zu\n",
                    parser.problem,
                    parser.problem_offset);
        }
        break;
    case YAML_SCANNER_ERROR: /* FALLTHROUGH */
    case YAML_PARSER_ERROR:
        if (parser.context) {
            t_error_set(T, "yaml_to_tags: %s error: %s at line %zu, column %zu\n"
                    "%s at line %zu, column %zu\n",
                    parser.error == YAML_SCANNER_ERROR ? "Scanner" : "Parser",
                    parser.context,
                    parser.context_mark.line + 1,
                    parser.context_mark.column + 1,
                    parser.problem,
                    parser.problem_mark.line + 1,
                    parser.problem_mark.column + 1);
        }
        else {
            t_error_set(T, "yaml_to_tags: %s error: %s at line %zu, column %zu\n",
                    parser.error == YAML_SCANNER_ERROR ? "Scanner" : "Parser",
                    parser.problem,
                    parser.problem_mark.line + 1,
                    parser.problem_mark.column + 1);
        }
        break;
    case YAML_NO_ERROR:       /* FALLTHROUGH */
    case YAML_COMPOSER_ERROR: /* FALLTHROUGH */
    case YAML_WRITER_ERROR:   /* FALLTHROUGH */
    case YAML_EMITTER_ERROR:
        t_error_set(T, "libyaml internal error\n"
                "bad error type while parsing: %s",
                parser.error == YAML_NO_ERROR ? "YAML_NO_ERROR" :
                parser.error == YAML_COMPOSER_ERROR ? "YAML_COMPOSER_ERROR" :
                parser.error == YAML_WRITER_ERROR ? "YAML_WRITER_ERROR" :
                parser.error == YAML_EMITTER_ERROR ? "YAML_EMITTER_ERROR" :
                "impossible");
        break;
    }

    yaml_event_delete(&event);
    yaml_parser_delete(&parser);
    return (T);
}


int
yaml_write_handler(void *data, unsigned char *buffer, size_t size)
{
    bool error = false;
    struct strbuf *sb;
    char *s;

    assert_not_null(data);
    assert_not_null(buffer);

    if (data == NULL || buffer == NULL)
        error = true;
    else {
        s = xcalloc(size + 1, sizeof(char));
        memcpy(s, buffer, size);
        sb = data;
        strbuf_add(sb, s, size);
    }

    if (error)
        return (0);
    else
        return (1);
}


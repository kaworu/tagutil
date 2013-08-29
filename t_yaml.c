/*
 * t_yaml.c
 *
 * yaml tagutil interface, using libyaml.
 */
#include <sys/param.h>

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/* libyaml headers */
#include "yaml.h"

#include "t_config.h"
#include "t_toolkit.h"
#include "t_file.h"
#include "t_yaml.h"


/*
 * libyaml emitter helper.
 */
yaml_write_handler_t t_yaml_whdl;

int
t_yaml_whdl(void *sb, unsigned char *buffer, size_t size)
{
	bool error = false;

	assert_not_null(sb);
	assert_not_null(buffer);

	if (sb == NULL || buffer == NULL)
		error = true;
	else
		(void)sbuf_bcat(sb, buffer, size);

	return (error ? 0 : 1);
}


char *
t_tags2yaml(struct t_file *file)
{
	yaml_emitter_t emitter;
	yaml_event_t event;
	struct sbuf *sb;
	char *s;
	struct t_taglist *T;
	struct t_tag *t;

	assert_not_null(file);

	/* create a comment header with the filename */
	(void)xasprintf(&s, "# %s\n", file->path);
	sb = sbuf_new_auto();
	if (sb == NULL)
		err(errno, "sbuf_new");
	(void)sbuf_cpy(sb, s);
	freex(s);

	/* Create the Emitter object. */
	if (!yaml_emitter_initialize(&emitter))
		goto emitter_error;

	yaml_emitter_set_output(&emitter, t_yaml_whdl, sb);
	yaml_emitter_set_unicode(&emitter, 1);

	/* Create and emit the STREAM-START event. */
	if (!yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING))
		goto event_error;
	if (!yaml_emitter_emit(&emitter, &event))
		goto emitter_error;

	/* Create and emit the DOCUMENT-START event. */
	if (!yaml_document_start_event_initialize(&event,
	    /* version directive */NULL,
	    /* tag_directives_start */NULL,
	    /* tag_directives_end */NULL,
	    /* implicit */0))
		goto event_error;
	if (!yaml_emitter_emit(&emitter, &event))
		goto emitter_error;

	/* Create and emit the SEQUENCE-START event. */
	if (!yaml_sequence_start_event_initialize(&event, /* anchor */NULL,
	    (yaml_char_t *)YAML_SEQ_TAG, /* implicit */1,
	    YAML_BLOCK_SEQUENCE_STYLE))
		goto event_error;
	if (!yaml_emitter_emit(&emitter, &event))
		goto emitter_error;

	T = file->get(file, NULL);
	if (T == NULL) {
		sbuf_delete(sb);
		return (NULL);
	}
	TAILQ_FOREACH(t, T->tags, entries) {
		/* Create and emit the MAPPING-START event. */
		if (!yaml_mapping_start_event_initialize(&event, /* anchor */NULL,
		    (yaml_char_t *)YAML_MAP_TAG, /* implicit */1,
		    YAML_BLOCK_MAPPING_STYLE))
			goto event_error;
		if (!yaml_emitter_emit(&emitter, &event))
			goto emitter_error;

		/* create and emit the SCALAR event for the key */
		if (!yaml_scalar_event_initialize(&event, /* anchor */NULL,
		    (yaml_char_t *)YAML_STR_TAG, (yaml_char_t *)t->key,
		    t->klen, /* plain_implicit */1, /* quoted_implicit */1,
		    YAML_PLAIN_SCALAR_STYLE))
			goto event_error;
		if (!yaml_emitter_emit(&emitter, &event))
			goto emitter_error;

		/* create and emit the SCALAR event for the value */
		if (!yaml_scalar_event_initialize(&event, /* anchor */NULL,
		    (yaml_char_t *)YAML_STR_TAG, (yaml_char_t *)t->val,
		    t->vlen, /* plain_implicit */1, /* quoted_implicit */1,
		    YAML_PLAIN_SCALAR_STYLE))
			goto event_error;
		if (!yaml_emitter_emit(&emitter, &event))
			goto emitter_error;

		/* Create and emit the MAPPING-END event. */
		if (!yaml_mapping_end_event_initialize(&event))
			goto event_error;
		if (!yaml_emitter_emit(&emitter, &event))
			goto emitter_error;
	}
	t_taglist_delete(T);

	/* Create and emit the SEQUENCE-END event. */
	if (!yaml_sequence_end_event_initialize(&event))
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

	if (sbuf_finish(sb) == -1)
		err(errno, "sbuf_finish");
	s = xstrdup(sbuf_data(sb));
	sbuf_delete(sb);

	return (s);

event_error:
	err(errno = ENOMEM, "t_tags2yaml: can't init event");
	/* NOTREACHED */
emitter_error:
	errx(-1, "t_tags2yaml: emit error");
}


/*
 * Our parser FSM. we only handle our YAML subset:
 *
 *  STREAM-START
 *      (DOCUMENT-START
 *          (SEQUENCE-START
 *              (MAPPING-START SCALAR SCALAR MAPPING-END)*
 *           SEQUENCE-END)?
 *       DOCUMENT-END)?
 *  STREAM-END
 */
struct t_yaml_fsm;
typedef void t_yaml_parse_func(struct t_yaml_fsm *FSM,
        const yaml_event_t *e);

t_yaml_parse_func t_yaml_parse_stream_start;

struct t_yaml_fsm {
    bool hungry;
    t_yaml_parse_func *handle;
    char *parsed_key;
    struct t_taglist *T;

    T_ERROR_MSG_MEMBER;
};

struct t_taglist *
t_yaml2tags(struct t_file *file, FILE *stream)
{
    struct t_yaml_fsm FSM;
    yaml_parser_t parser;
    yaml_event_t event;

    assert_not_null(stream);
    assert_not_null(file);
    t_error_clear(file);

    (void)memset(&FSM, 0, sizeof(FSM));
    t_error_init(&FSM);

    if (!yaml_parser_initialize(&parser))
        goto parser_error;
    yaml_parser_set_input_file(&parser, stream);

    FSM.handle = t_yaml_parse_stream_start;
    do {
        if (!yaml_parser_parse(&parser, &event))
            goto parser_error;
        FSM.handle(&FSM, &event);
    } while (FSM.hungry);

    if (t_error_msg(&FSM)) {
        t_error_set(file, "YAML parser: %s", t_error_msg(&FSM));
        goto cleanup;
    }

    yaml_event_delete(&event);
    yaml_parser_delete(&parser);
    return (FSM.T);

parser_error:
    switch (parser.error) {
    case YAML_MEMORY_ERROR:
        t_error_set(file, "t_yaml2tags: YAML Parser (ENOMEM)");
        break;
    case YAML_READER_ERROR:
        if (parser.problem_value != -1) {
            t_error_set(file, "t_yaml2tags: Reader error: %s: #%X at %zu\n",
                    parser.problem,
                    parser.problem_value,
                    parser.problem_offset);
        }
        else {
            t_error_set(file, "t_yaml2tags: Reader error: %s at %zu\n",
                    parser.problem,
                    parser.problem_offset);
        }
        break;
    case YAML_SCANNER_ERROR: /* FALLTHROUGH */
    case YAML_PARSER_ERROR:
        if (parser.context) {
            t_error_set(file, "t_yaml2tags: %s error: %s at line %zu, column %zu\n"
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
            t_error_set(file, "t_yaml2tags: %s error: %s at line %zu, column %zu\n",
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
        t_error_set(file, "libyaml internal error\n"
                "bad error type while parsing: %s",
                parser.error == YAML_NO_ERROR ? "YAML_NO_ERROR" :
                parser.error == YAML_COMPOSER_ERROR ? "YAML_COMPOSER_ERROR" :
                parser.error == YAML_WRITER_ERROR ? "YAML_WRITER_ERROR" :
                parser.error == YAML_EMITTER_ERROR ? "YAML_EMITTER_ERROR" :
                "impossible");
        break;
    }

cleanup:
    yaml_event_delete(&event);
    yaml_parser_delete(&parser);
    t_error_clear(&FSM);
    freex(FSM.parsed_key);
    t_taglist_delete(FSM.T);
    return (NULL);
}


/*
 * more stuff for the parsing functions
 */
static const char *t_yaml_event_str[] = {
    [YAML_NO_EVENT]             = "YAML_NO_EVENT",
    [YAML_STREAM_START_EVENT]   = "YAML_STREAM_START_EVENT",
    [YAML_STREAM_END_EVENT]     = "YAML_STREAM_END_EVENT",
    [YAML_DOCUMENT_START_EVENT] = "YAML_DOCUMENT_START_EVENT",
    [YAML_DOCUMENT_END_EVENT]   = "YAML_DOCUMENT_END_EVENT",
    [YAML_ALIAS_EVENT]          = "YAML_ALIAS_EVENT",
    [YAML_SCALAR_EVENT]         = "YAML_SCALAR_EVENT",
    [YAML_SEQUENCE_START_EVENT] = "YAML_SEQUENCE_START_EVENT",
    [YAML_SEQUENCE_END_EVENT]   = "YAML_SEQUENCE_END_EVENT",
    [YAML_MAPPING_START_EVENT]  = "YAML_MAPPING_START_EVENT",
    [YAML_MAPPING_END_EVENT]    = "YAML_MAPPING_END_EVENT",
};

t_yaml_parse_func t_yaml_parse_document_start;
t_yaml_parse_func t_yaml_parse_sequence_start;
t_yaml_parse_func t_yaml_parse_mapping_start;
t_yaml_parse_func t_yaml_parse_scalar_key;
t_yaml_parse_func t_yaml_parse_scalar_value;
t_yaml_parse_func t_yaml_parse_mapping_end;
t_yaml_parse_func t_yaml_parse_document_end;
t_yaml_parse_func t_yaml_parse_stream_end;
t_yaml_parse_func t_yaml_parse_nop;


void
t_yaml_parse_stream_start(struct t_yaml_fsm *FSM, const yaml_event_t *e)
{

	assert_not_null(FSM);
	assert_not_null(e);

	if (e->type == YAML_STREAM_START_EVENT) {
		if ((FSM->T = t_taglist_new()) == NULL)
			err(ENOMEM, "malloc");
		FSM->handle = t_yaml_parse_document_start;
		FSM->hungry = true;
	}
	else {
		t_error_set(FSM, "expected %s, got %s",
				t_yaml_event_str[YAML_STREAM_START_EVENT],
				t_yaml_event_str[e->type]);
		FSM->hungry = false;
		FSM->handle = t_yaml_parse_nop;
	}
}


void
t_yaml_parse_document_start(struct t_yaml_fsm *FSM,
    const yaml_event_t *e)
{

    assert_not_null(FSM);
    assert_not_null(e);

    switch (e->type) {
    case YAML_DOCUMENT_START_EVENT:
        FSM->handle = t_yaml_parse_sequence_start;
        break;
    case YAML_STREAM_END_EVENT:
        FSM->hungry = false;
        FSM->handle = t_yaml_parse_nop;
        break;
    default:
        t_error_set(FSM, "expected %s or %s, got %s",
                t_yaml_event_str[YAML_DOCUMENT_START_EVENT],
                t_yaml_event_str[YAML_STREAM_END_EVENT],
                t_yaml_event_str[e->type]);
        FSM->hungry = false;
        FSM->handle = t_yaml_parse_nop;
        break;
    }
}


void
t_yaml_parse_sequence_start(struct t_yaml_fsm *FSM,
    const yaml_event_t *e)
{

    assert_not_null(FSM);
    assert_not_null(e);

    switch (e->type) {
    case YAML_SEQUENCE_START_EVENT:
        FSM->handle = t_yaml_parse_mapping_start;
        break;
    case YAML_DOCUMENT_END_EVENT:
        FSM->handle = t_yaml_parse_stream_end;
        break;
    default:
        t_error_set(FSM, "expected %s or %s, got %s",
                t_yaml_event_str[YAML_SEQUENCE_START_EVENT],
                t_yaml_event_str[YAML_DOCUMENT_END_EVENT],
                t_yaml_event_str[e->type]);
        FSM->hungry = false;
        FSM->handle = t_yaml_parse_nop;
        break;
    }
}


void
t_yaml_parse_mapping_start(struct t_yaml_fsm *FSM,
    const yaml_event_t *e)
{

    assert_not_null(FSM);
    assert_not_null(e);

    switch (e->type) {
    case YAML_MAPPING_START_EVENT:
        FSM->handle = t_yaml_parse_scalar_key;
        break;
    case YAML_SEQUENCE_END_EVENT:
        FSM->handle = t_yaml_parse_document_end;
        break;
    default:
        t_error_set(FSM, "expected %s or %s, got %s",
                t_yaml_event_str[YAML_MAPPING_START_EVENT],
                t_yaml_event_str[YAML_SEQUENCE_END_EVENT],
                t_yaml_event_str[e->type]);
        FSM->hungry = false;
        FSM->handle = t_yaml_parse_nop;
        break;
    }
}


void
t_yaml_parse_scalar_key(struct t_yaml_fsm *FSM,
    const yaml_event_t *e)
{

    assert_not_null(FSM);
    assert_not_null(e);

    if (e->type == YAML_SCALAR_EVENT) {
        FSM->parsed_key = xcalloc(e->data.scalar.length + 1, sizeof(char));
        (void)memcpy(FSM->parsed_key, e->data.scalar.value, e->data.scalar.length);
        FSM->handle = t_yaml_parse_scalar_value;
    }
    else {
        t_error_set(FSM, "expected %s (key), got %s",
                t_yaml_event_str[YAML_SCALAR_EVENT],
                t_yaml_event_str[e->type]);
        FSM->hungry = false;
        FSM->handle = t_yaml_parse_nop;
    }
}


void
t_yaml_parse_scalar_value(struct t_yaml_fsm *FSM,
    const yaml_event_t *e)
{
	char *val;

	assert_not_null(FSM);
	assert_not_null(e);

	if (e->type == YAML_SCALAR_EVENT) {
		val = xcalloc(e->data.scalar.length + 1, sizeof(char));
		(void)memcpy(val, e->data.scalar.value, e->data.scalar.length);
		if ((t_taglist_insert(FSM->T, FSM->parsed_key, val)) == -1)
			err(ENOMEM, "malloc");
		freex(FSM->parsed_key);
		freex(val);
		FSM->handle = t_yaml_parse_mapping_end;
	} else {
		t_error_set(FSM, "expected %s (value), got %s",
		    t_yaml_event_str[YAML_SCALAR_EVENT],
		    t_yaml_event_str[e->type]);
		FSM->hungry = false;
		FSM->handle = t_yaml_parse_nop;
	}
}


void
t_yaml_parse_mapping_end(struct t_yaml_fsm *FSM,
    const yaml_event_t *e)
{

    assert_not_null(FSM);
    assert_not_null(e);

    if (e->type == YAML_MAPPING_END_EVENT)
        FSM->handle = t_yaml_parse_mapping_start;
    else {
        t_error_set(FSM, "expected %s, got %s",
                t_yaml_event_str[YAML_MAPPING_END_EVENT],
                t_yaml_event_str[e->type]);
        FSM->hungry = false;
        FSM->handle = t_yaml_parse_nop;
    }
}


void
t_yaml_parse_document_end(struct t_yaml_fsm *FSM,
    const yaml_event_t *e)
{

    assert_not_null(FSM);
    assert_not_null(e);

    if (e->type == YAML_DOCUMENT_END_EVENT)
        FSM->handle = t_yaml_parse_stream_end;
    else {
        t_error_set(FSM, "expected %s, got %s",
                t_yaml_event_str[YAML_DOCUMENT_END_EVENT],
                t_yaml_event_str[e->type]);
        FSM->hungry = false;
        FSM->handle = t_yaml_parse_nop;
    }
}


void
t_yaml_parse_stream_end(struct t_yaml_fsm *FSM,
    const yaml_event_t *e)
{

    assert_not_null(FSM);
    assert_not_null(e);

    if (e->type == YAML_STREAM_END_EVENT) {
        FSM->handle = t_yaml_parse_nop;
        FSM->hungry = false;
        /* ouf! finally! :) */
    }
    else {
        t_error_set(FSM, "expected %s, got %s",
                t_yaml_event_str[YAML_STREAM_END_EVENT],
                t_yaml_event_str[e->type]);
        FSM->hungry = false;
        FSM->handle = t_yaml_parse_nop;
    }
}


void
t_yaml_parse_nop(_t__unused struct t_yaml_fsm *FSM,
    _t__unused const yaml_event_t *e)
{

    assert_fail();
}

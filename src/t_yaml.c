/*
 * t_yaml.c
 *
 * yaml tagutil interface, using libyaml.
 *
 * basically implement both t_yaml2tags and t_tags2yaml.
 *
 * XXX: t_yaml2tags use a Finite State Machine that should be refactored into
 * something simpler and cleaner. At the same time t_error should be dropped
 * (it's the only code that still use it).
 */
#include <string.h>
#include <stdlib.h>

/* libyaml headers */
#include "yaml.h"

#include "t_config.h"
#include "t_toolkit.h"
#include "t_taglist.h"
#include "t_format.h"



/* t_error handling macros */
/* used for any struct that need to behave like a t_error */
#define	T_ERROR_MSG_MEMBER	char *t__errmsg /* t__deprecated XXX: whine too much */
/* error message getter */
#define	t_error_msg(o)	((o)->t__errmsg)
/* initializer */
#define	t_error_init(o)	do { t_error_msg(o) = NULL; } while (/*CONSTCOND*/0)
/* set the error message (with printflike syntax) */
#define	t_error_set(o, fmt, ...)                                         \
	do {                                                             \
		if (asprintf(&t_error_msg(o), fmt, ##__VA_ARGS__) == -1) \
			err(EXIT_FAILURE, "asprintf");                   \
	} while (/*CONSTCOND*/0)
/* reset the error message. free it if needed, set to NULL */
#define	t_error_clear(o) \
    do { free(t_error_msg(o)); t_error_init(o); } while (/*CONSTCOND*/0)


static const char libid[]   = "libyaml";
static const char fileext[] = "yml";


struct t_format		*t_yaml_format(void);

static char		*t_tags2yaml(const struct t_taglist *tlist, const char *path);
static struct t_taglist	*t_yaml2tags(FILE *fp, char **errmsg_p);
/*
 * libyaml emitter helper.
 */
yaml_write_handler_t t_yaml_whdl;

struct t_format *
t_yaml_format(void)
{
	static struct t_format fmt = {
		.libid		= libid,
		.fileext	= fileext,
		.desc		=
		    "YAML - YAML Ain't Markup Language",
		.tags2fmt	= t_tags2yaml,
		.fmt2tags	= t_yaml2tags,
	};

	return (&fmt);
}


int
t_yaml_whdl(void *sb, unsigned char *buffer, size_t size)
{

	assert_not_null(sb);
	assert_not_null(buffer);

	if (sb == NULL || buffer == NULL)
		return (0); /* error */
	else
		(void)sbuf_bcat(sb, buffer, size);
	
	return (1); /* success */
}


static char *
t_tags2yaml(const struct t_taglist *tlist, const char *path)
{
	yaml_emitter_t emitter;
	yaml_event_t event;
	struct sbuf *sb;
	const struct t_tag *t;
	char *ret;

	assert_not_null(tlist);

	sb = sbuf_new_auto();
	if (sb == NULL)
		return (NULL);

	if (path != NULL) {
		/* create a comment header with the filename */
		(void)sbuf_printf(sb, "# %s\n", path);
	}

	/* Create the Emitter object. */
	if (!yaml_emitter_initialize(&emitter))
		goto emitter_error_label;

	yaml_emitter_set_output(&emitter, t_yaml_whdl, sb);
	yaml_emitter_set_unicode(&emitter, 1);

	/* Create and emit the STREAM-START event. */
	if (!yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING))
		goto event_error_label;
	if (!yaml_emitter_emit(&emitter, &event))
		goto emitter_error_label;

	/* Create and emit the DOCUMENT-START event. */
	if (!yaml_document_start_event_initialize(&event,
	    /* version directive */NULL,
	    /* tag_directives_start */NULL,
	    /* tag_directives_end */NULL,
	    /* implicit */0))
		goto event_error_label;
	if (!yaml_emitter_emit(&emitter, &event))
		goto emitter_error_label;

	/* Create and emit the SEQUENCE-START event. */
	if (!yaml_sequence_start_event_initialize(&event, /* anchor */NULL,
	    (yaml_char_t *)YAML_SEQ_TAG, /* implicit */1,
	    YAML_BLOCK_SEQUENCE_STYLE))
		goto event_error_label;
	if (!yaml_emitter_emit(&emitter, &event))
		goto emitter_error_label;

	TAILQ_FOREACH(t, tlist->tags, entries) {
		/* Create and emit the MAPPING-START event. */
		if (!yaml_mapping_start_event_initialize(&event, /* anchor */NULL,
		    (yaml_char_t *)YAML_MAP_TAG, /* implicit */1,
		    YAML_BLOCK_MAPPING_STYLE))
			goto event_error_label;
		if (!yaml_emitter_emit(&emitter, &event))
			goto emitter_error_label;

		/* create and emit the SCALAR event for the key */
		if (!yaml_scalar_event_initialize(&event, /* anchor */NULL,
		    (yaml_char_t *)YAML_STR_TAG, (yaml_char_t *)t->key,
		    t->klen, /* plain_implicit */1, /* quoted_implicit */1,
		    YAML_PLAIN_SCALAR_STYLE))
			goto event_error_label;
		if (!yaml_emitter_emit(&emitter, &event))
			goto emitter_error_label;

		/* create and emit the SCALAR event for the value */
		if (!yaml_scalar_event_initialize(&event, /* anchor */NULL,
		    (yaml_char_t *)YAML_STR_TAG, (yaml_char_t *)t->val,
		    t->vlen, /* plain_implicit */1, /* quoted_implicit */1,
		    YAML_PLAIN_SCALAR_STYLE))
			goto event_error_label;
		if (!yaml_emitter_emit(&emitter, &event))
			goto emitter_error_label;

		/* Create and emit the MAPPING-END event. */
		if (!yaml_mapping_end_event_initialize(&event))
			goto event_error_label;
		if (!yaml_emitter_emit(&emitter, &event))
			goto emitter_error_label;
	}

	/* Create and emit the SEQUENCE-END event. */
	if (!yaml_sequence_end_event_initialize(&event))
		goto event_error_label;
	if (!yaml_emitter_emit(&emitter, &event))
		goto emitter_error_label;

	/* Create and emit the DOCUMENT-END event. */
	if (!yaml_document_end_event_initialize(&event, /* implicit */1))
		goto event_error_label;
	if (!yaml_emitter_emit(&emitter, &event))
		goto emitter_error_label;

	/* Create and emit the STREAM-END event. */
	if (!yaml_stream_end_event_initialize(&event))
		goto event_error_label;
	if (!yaml_emitter_emit(&emitter, &event))
		goto emitter_error_label;

	/* Destroy the Emitter object. */
	yaml_emitter_delete(&emitter);
	yaml_event_delete(&event);

	if (sbuf_finish(sb) == -1) {
		warn("sbuf_finish");
		sbuf_delete(sb);
		return (NULL);
	}

	ret = strdup(sbuf_data(sb));
	sbuf_delete(sb);
	return (ret);
	/* NOTREACHED */
event_error_label:
	sbuf_delete(sb);
	errno = ENOMEM;
	return (NULL);
	/* NOTREACHED */
emitter_error_label:
	warnx("t_tags2yaml: emit error");
	ABANDON_SHIP();
}


/*
 * Our parser FSM. we only handle this YAML subset:
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
typedef void t_yaml_parse_func(struct t_yaml_fsm *FSM, const yaml_event_t *e);

t_yaml_parse_func t_yaml_parse_stream_start;

struct t_yaml_fsm {
	t_yaml_parse_func	*handle;
	char			*parsed_key;
	struct t_taglist	*tlist;
	int	hungry;
	T_ERROR_MSG_MEMBER;
};


static struct t_taglist *
t_yaml2tags(FILE *fp, char **errmsg_p)
{
	struct t_yaml_fsm FSM;
	yaml_parser_t parser;
	yaml_event_t event;
	char *errmsg = NULL;

	assert_not_null(fp);

	(void)memset(&FSM, 0, sizeof(FSM));
	t_error_init(&FSM);

	if (!yaml_parser_initialize(&parser))
		goto parser_error_label;
	yaml_parser_set_input_file(&parser, fp);

	FSM.handle = t_yaml_parse_stream_start;
	do {
		if (!yaml_parser_parse(&parser, &event))
			goto parser_error_label;
		FSM.handle(&FSM, &event);
		yaml_event_delete(&event);
	} while (FSM.hungry);

	if (t_error_msg(&FSM)) {
		xasprintf(&errmsg, "YAML parser: %s", t_error_msg(&FSM));
		goto cleanup_label;
	}

	yaml_parser_delete(&parser);
	return (FSM.tlist);
	/* NOTREACHED */

parser_error_label:
	switch (parser.error) {
		case YAML_MEMORY_ERROR:
			xasprintf(&errmsg, "t_yaml2tags: YAML Parser (ENOMEM)");
			break;
		case YAML_READER_ERROR:
			if (parser.problem_value != -1) {
				xasprintf(&errmsg, "t_yaml2tags: Reader error: %s: #%X at %zu\n",
				    parser.problem, parser.problem_value, parser.problem_offset);
			} else {
				xasprintf(&errmsg, "t_yaml2tags: Reader error: %s at %zu\n",
				    parser.problem, parser.problem_offset);
			}
			break;
		case YAML_SCANNER_ERROR: /* FALLTHROUGH */
		case YAML_PARSER_ERROR:
			if (parser.context) {
				xasprintf(&errmsg, "t_yaml2tags: %s error: %s at line %zu, column %zu\n"
				    "%s at line %zu, column %zu\n",
				    parser.error == YAML_SCANNER_ERROR ? "Scanner" : "Parser",
				    parser.context, parser.context_mark.line + 1,
				    parser.context_mark.column + 1, parser.problem,
				    parser.problem_mark.line + 1, parser.problem_mark.column + 1);
			} else {
				xasprintf(&errmsg, "t_yaml2tags: %s error: %s at line %zu, column %zu\n",
				    parser.error == YAML_SCANNER_ERROR ? "Scanner" : "Parser",
				    parser.problem, parser.problem_mark.line + 1,
				    parser.problem_mark.column + 1);
			}
			break;
		case YAML_NO_ERROR:       /* FALLTHROUGH */
		case YAML_COMPOSER_ERROR: /* FALLTHROUGH */
		case YAML_WRITER_ERROR:   /* FALLTHROUGH */
		case YAML_EMITTER_ERROR:
			xasprintf(&errmsg, "libyaml internal error\n"
			    "bad error type while parsing: %s",
			    parser.error == YAML_NO_ERROR ? "YAML_NO_ERROR" :
			    parser.error == YAML_COMPOSER_ERROR ? "YAML_COMPOSER_ERROR" :
			    parser.error == YAML_WRITER_ERROR ? "YAML_WRITER_ERROR" :
			    parser.error == YAML_EMITTER_ERROR ? "YAML_EMITTER_ERROR" :
			    "impossible");
			break;
	}

cleanup_label:
	yaml_event_delete(&event);
	yaml_parser_delete(&parser);
	t_error_clear(&FSM);
	free(FSM.parsed_key);
	t_taglist_delete(FSM.tlist);

	if (errmsg_p != NULL)
		*errmsg_p = errmsg;
	else
		free(errmsg);

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
		if ((FSM->tlist = t_taglist_new()) == NULL)
			err(EXIT_FAILURE, "malloc");
		FSM->handle = t_yaml_parse_document_start;
		FSM->hungry = 1;
	}
	else {
		t_error_set(FSM, "expected %s, got %s",
				t_yaml_event_str[YAML_STREAM_START_EVENT],
				t_yaml_event_str[e->type]);
		FSM->hungry = 0;
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
        FSM->hungry = 0;
        FSM->handle = t_yaml_parse_nop;
        break;
    default:
        t_error_set(FSM, "expected %s or %s, got %s",
                t_yaml_event_str[YAML_DOCUMENT_START_EVENT],
                t_yaml_event_str[YAML_STREAM_END_EVENT],
                t_yaml_event_str[e->type]);
        FSM->hungry = 0;
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
        FSM->hungry = 0;
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
        FSM->hungry = 0;
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
        FSM->parsed_key = calloc(e->data.scalar.length + 1, sizeof(char));
	if (FSM->parsed_key == NULL)
		err(EXIT_FAILURE, "calloc");
        (void)memcpy(FSM->parsed_key, e->data.scalar.value, e->data.scalar.length);
        FSM->handle = t_yaml_parse_scalar_value;
    }
    else {
        t_error_set(FSM, "expected %s (key), got %s",
                t_yaml_event_str[YAML_SCALAR_EVENT],
                t_yaml_event_str[e->type]);
        FSM->hungry = 0;
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
		val = calloc(e->data.scalar.length + 1, sizeof(char));
		if (val == NULL)
			err(EXIT_FAILURE, "calloc");
		(void)memcpy(val, e->data.scalar.value, e->data.scalar.length);
		/* FIXME: convert to UTF-8 */
		if ((t_taglist_insert(FSM->tlist, FSM->parsed_key, val)) == -1)
			err(EXIT_FAILURE, "malloc");
		free(FSM->parsed_key);
		FSM->parsed_key = NULL;
		free(val);
		val = NULL;
		FSM->handle = t_yaml_parse_mapping_end;
	} else {
		t_error_set(FSM, "expected %s (value), got %s",
		    t_yaml_event_str[YAML_SCALAR_EVENT],
		    t_yaml_event_str[e->type]);
		FSM->hungry = 0;
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
        FSM->hungry = 0;
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
        FSM->hungry = 0;
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
        FSM->hungry = 0;
        /* ouf! finally! :) */
    }
    else {
        t_error_set(FSM, "expected %s, got %s",
                t_yaml_event_str[YAML_STREAM_END_EVENT],
                t_yaml_event_str[e->type]);
        FSM->hungry = 0;
        FSM->handle = t_yaml_parse_nop;
    }
}


void
t_yaml_parse_nop(t__unused struct t_yaml_fsm *FSM,
    t__unused const yaml_event_t *e)
{

    ABANDON_SHIP();
}

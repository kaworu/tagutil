/*
 * t_json.c
 *
 * json tagutil interface, using jansson.
 */
#include <string.h>
#include <stdlib.h>

/* jansson headers */
#include "jansson.h"
/*
 * json_array_foreach macro is new in 2.5. We could test JANSSON_VERSION_HEX but
 * that would be ugly.
 */
#ifndef json_array_foreach
#define	json_array_foreach(array, index, value) \
	for (index = 0; index < json_array_size(array) && (value = json_array_get(array, index)); index++)
#endif

#include "t_config.h"
#include "t_toolkit.h"
#include "t_taglist.h"
#include "t_format.h"


static const char libid[]   = "jansson";
static const char fileext[] = "json";


struct t_format		*t_json_format(void);

static char		*t_tags2json(const struct t_taglist *tlist, const char *path);
static struct t_taglist	*t_json2tags(FILE *fp, char **errmsg_p);


struct t_format *
t_json_format(void)
{
	static struct t_format fmt = {
		.libid		= libid,
		.fileext	= fileext,
		.desc		=
		    "JSON - JavaScript Object Notation",
		.tags2fmt	= t_tags2json,
		.fmt2tags	= t_json2tags,
	};

	return (&fmt);
}


static char *
t_tags2json(const struct t_taglist *tlist, t__unused const char *path)
{
	json_t *root = NULL, *obj = NULL;
	const struct t_tag *t;
	char *ret = NULL;

	assert(tlist != NULL);

	if ((root = json_array()) == NULL)
		goto error_label;

	TAILQ_FOREACH(t, tlist->tags, entries) {
		if ((obj = json_object()) == NULL)
			goto error_label;
		if (json_object_set_new_nocheck(obj, t->key,
		    json_string_nocheck(t->val)) == -1) {
			goto error_label;
		}
		if (json_array_append(root, obj) == -1)
			goto error_label;
		json_decref(obj);
		obj = NULL;
	}

	ret = json_dumps(root, JSON_COMPACT);
	/* FALLTHROUGH */
error_label:
	if (root != NULL)
		json_decref(root);
	if (obj != NULL)
		json_decref(obj);

	return (ret);
}


/*
 * We only handle this JSON subset:
 *
 *  [
 *      { "key" : "value" },
 *      { "key" : "value" },
 *      ...
 *  ]
 */
static struct t_taglist *
t_json2tags(FILE *fp, char **errmsg_p)
{
	size_t idx;
	struct t_taglist *ret = NULL, *tlist = NULL;
	json_t *root = NULL, *obj, *val;
	json_error_t error;
	char *errmsg = NULL;

	assert(fp != NULL);

	/* parse and load the JSON */
	root = json_loadf(fp, JSON_DISABLE_EOF_CHECK, &error);
	if (root == NULL) {
		xasprintf(&errmsg, "json parsing error on line %d: %s\n",
		    error.line, error.text);
		goto error_label;
	}
	if (!json_is_array(root)) {
		xasprintf(&errmsg, "json parsing error: root is not an array");
		goto error_label;
	}

	if ((tlist = t_taglist_new()) == NULL) {
		xasprintf(&errmsg, "%s", strerror(errno));
		goto error_label;
	}

	json_array_foreach(root, idx, obj) {
		const char *key;
		if (json_object_size(obj) != 1) {
			/* this also test if obj is an object */
			xasprintf(&errmsg, "json parsing error: element#%zu is "
			    "not an object (or has many elements)", idx);
			goto error_label;
		}

		json_object_foreach(obj, key, val) {
			char buf[21]; /* "-9223372036854775808"NUL (LLONG_MIN) */
			const char *vstr = buf;

			if (json_is_string(val))
				vstr = json_string_value(val);
			else if (json_is_integer(val)) {
				/*
				 * XXX: ugly ugly ugly we should just
				 * xasprintf() and strdup when
				 * json_is_string(val)
				 */
				int n = snprintf(buf, sizeof(buf),
				    "%"JSON_INTEGER_FORMAT,
				    json_integer_value(val));
				if (n >= sizeof(buf)) {
					/* json_int_t should be `long long' (at
					 * max) and sizeof(long long) > 8 is
					 * unlikely. Maybe both
					 * json_integer_value() and
					 * JSON_INTEGER_FORMAT are lying, or I
					 * just screwed something.
					 */
					ABANDON_SHIP();
				}
			} else {
				xasprintf(&errmsg, "json parsing error: object#"
				    "%zu's value is not a string nor a number",
				    idx);
				goto error_label;
			}
			if (t_taglist_insert(tlist, key, vstr) == -1) {
				xasprintf(&errmsg, "%s", strerror(errno));
				goto error_label;
			}
		}
	}

	/* all went well. now switch ret and tlist */
	ret   = tlist;
	tlist = NULL;

	/* FALLTHROUGH */
error_label:
	if (root != NULL)
		json_decref(root);
	if (tlist != NULL)
		t_taglist_delete(tlist);

	if (errmsg_p != NULL)
		*errmsg_p = errmsg;
	else
		free(errmsg);
	return (ret);
}

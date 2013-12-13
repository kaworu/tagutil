/*
 * t_json.c
 *
 * json tagutil interface, using jansson.
 */
#include <sys/param.h>

#include <string.h>
#include <stdlib.h>

/* jansson headers */
#include "jansson.h"

#include "t_config.h"
#include "t_toolkit.h"
#include "t_taglist.h"
#include "t_tune.h"
#include "t_json.h"


char *
t_tags2json(const struct t_taglist *tlist, const char *path)
{
	json_t *root = NULL, *obj = NULL;
	const struct t_tag *t;
	char *ret = NULL;

	assert_not_null(tlist);

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
 * Our parser FSM. we only handle this JSON subset:
 *
 *  [
 *      { "key" : "value" },
 *      { "key" : "value" },
 *      ...
 *  ]
 */
struct t_taglist *
t_json2tags(FILE *fp, char **errmsg_p)
{
	assert(0 && "Unimplemented");
	/* NOTREACHED */
}

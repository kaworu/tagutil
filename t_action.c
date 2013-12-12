/*
 * t_action.c
 *
 * tagutil actions.
 *
 * Actions are all defined in this file. Complex action have implementations in
 * their own file(s) to avoid bloating this one (like load, edit, rename) while
 * simple action are implemented in this one. It could have an overall better
 * design (like backend for example) but it is intentionally "big", "ugly" and
 * simple to discourage anyone to add more action (so tagutil stay small).
 */
#include "t_config.h"
#include "t_action.h"

#include "t_backend.h"
#include "t_taglist.h"
#include "t_tune.h"
#include "t_editor.h"
#include "t_loader.h"
#include "t_renamer.h"
#include "t_yaml.h"


struct t_action_token {
	const char		*word;
	enum t_actionkind 	 kind;
	int			 argc;
};


/* keep this array sorted, as it is used with bsearch(3) */
static struct t_action_token t_action_keywords[] = {
	{ .word = "add",	.kind = T_ACTION_ADD,		.argc = 1 },
	{ .word = "backend",	.kind = T_ACTION_BACKEND,	.argc = 0 },
	{ .word = "clear",	.kind = T_ACTION_CLEAR,		.argc = 1 },
	{ .word = "edit",	.kind = T_ACTION_EDIT,		.argc = 0 },
	{ .word = "load",	.kind = T_ACTION_LOAD,		.argc = 1 },
	{ .word = "print",	.kind = T_ACTION_PRINT,		.argc = 0 },
	{ .word = "rename",	.kind = T_ACTION_RENAME,	.argc = 1 },
	{ .word = "set",	.kind = T_ACTION_SET,		.argc = 1 },
};


/*
 * create a new action.
 * return NULL on error and set errno to ENOMEM or EINVAL
 */
static struct t_action	*t_action_new(enum t_actionkind kind, const char *arg);
/* free an action and all internal ressources */
static void		 t_action_delete(struct t_action *victim);

/* action methods */
static int	t_action_add(struct t_action *self, struct t_tune *tune);
static int	t_action_backend(struct t_action *self, struct t_tune *tune);
static int	t_action_clear(struct t_action *self, struct t_tune *tune);
static int	t_action_edit(struct t_action *self, struct t_tune *tune);
static int	t_action_load(struct t_action *self, struct t_tune *tune);
static int	t_action_print(struct t_action *self, struct t_tune *tune);
static int	t_action_rename(struct t_action *self, struct t_tune *tune);
static int	t_action_set(struct t_action *self, struct t_tune *tune);

/* used to search in the t_action_keywords array */
static int	t_action_token_cmp(const void *vstr, const void *vtoken);


struct t_actionQ *
t_actionQ_new(int *argc_p, char ***argv_p)
{
	int argc;
	char **argv;
	struct t_action  *a;
	struct t_actionQ *aQ;

	assert_not_null(argc_p);
	assert_not_null(argv_p);

	argc = *argc_p;
	argv = *argv_p;
	aQ = malloc(sizeof(struct t_actionQ));
	if (aQ == NULL)
		goto error_label;
	TAILQ_INIT(aQ);

	while (argc > 0) {
		char	*arg;
		struct t_action_token *t;

		t = bsearch(*argv, t_action_keywords,
		    NELEM(t_action_keywords), sizeof(*t_action_keywords),
		    t_action_token_cmp);
		if (t == NULL) {
			/* it doesn't look like an option */
			break;
		}
		if (t->argc == 0)
			arg = NULL;
		else if (t->argc == 1) {
			arg = strchr(*argv, ':');
			if (arg == NULL) {
				warnx("option %s requires an argument", t->word);
				errno = EINVAL;
				goto error_label;
				/* NOTREACHED */
			}
			arg++; /* skip the `:' char */
		} else {
			/* t->argc > 1 is unsuported */
			ABANDON_SHIP();
		}

		a = t_action_new(t->kind, arg);
		if (a == NULL)
			goto error_label;
		TAILQ_INSERT_TAIL(aQ, a, entries);

		argc--;
		argv++;
	}

	if (TAILQ_EMPTY(aQ)) {
		/* no action given, fallback to default which is print */
		a = t_action_new(T_ACTION_PRINT, NULL);
		if (a == NULL)
			goto error_label;
		TAILQ_INSERT_TAIL(aQ, a, entries);
	}

	*argc_p = argc;
	*argv_p = argv;
	return (aQ);
	/* NOTREACHED */
error_label:
	t_actionQ_delete(aQ);
	assert(errno == EINVAL || errno == ENOMEM);
	return (NULL);
}


void
t_actionQ_delete(struct t_actionQ *aQ)
{

	if (aQ != NULL) {
		struct t_action *victim, *next;
		victim = TAILQ_FIRST(aQ);
		while (victim != NULL) {
			next = TAILQ_NEXT(victim, entries);
			t_action_delete(victim);
			victim = next;
		}
	}
	free(aQ);
}


/*
 * create a new action of the given type.
 *
 * @param kind
 *   The action kind.
 *
 * @param arg
 *   The argument for this action kind. If the action kind require an argument,
 *   arg cannot be NULL (this should be enforced by the caller).
 *
 * @return
 *   A new action or NULL or error. On error, errno is set to either EINVAL or
 *   ENOMEM.
 */
static struct t_action *
t_action_new(enum t_actionkind kind, const char *arg)
{
	char *key = NULL, *val, *eq;
	struct t_action *a;

	a = calloc(1, sizeof(struct t_action));
	if (a == NULL)
		goto error_label;
	a->kind = kind;

	switch (a->kind) {
	case T_ACTION_ADD:
		assert_not_null(arg);
		if ((key = strdup(arg)) == NULL)
			goto error_label;
		eq = strchr(key, '=');
		if (eq == NULL) {
			errno = EINVAL;
			warn("add: missing '='");
			goto error_label;
		}
		*eq = '\0';
		val = eq + 1;
		if ((a->opaque = t_tag_new(key, val)) == NULL)
			goto error_label;
		free(key);
		key = NULL;
		a->write = 1;
		a->apply = t_action_add;
		break;
	case T_ACTION_BACKEND:
		a->apply = t_action_backend;
		break;
	case T_ACTION_CLEAR:
		assert_not_null(arg);
		if (strlen(arg) > 0) {
			a->opaque = strdup(arg);
			if (a->opaque == NULL)
				goto error_label;
		}
		a->write = 1;
		a->apply = t_action_clear;
		break;
	case T_ACTION_EDIT:
		a->write = 1;
		a->apply = t_action_edit;
		break;
	case T_ACTION_LOAD:
		assert_not_null(arg);
		a->opaque = strdup(arg);
		if (a->opaque == NULL)
			goto error_label;
		a->write = 1;
		a->apply = t_action_load;
		break;
	case T_ACTION_PRINT:
		a->apply = t_action_print;
		break;
	case T_ACTION_RENAME:
		assert_not_null(arg);
		if (strlen(arg) == 0) {
			errno = EINVAL;
			warn("empty rename pattern");
			goto error_label;
		}
		a->opaque = t_rename_parse(arg);
		if (a->opaque == NULL) {
			errno = EINVAL;
			warn("rename: bad rename pattern");
			goto error_label;
		}
		a->write = 1;
		a->apply = t_action_rename;
		break;
	case T_ACTION_SET:
		assert_not_null(arg);
		if ((key = strdup(arg)) == NULL)
			goto error_label;
		eq = strchr(key, '=');
		if (eq == NULL) {
			errno = EINVAL;
			warn("set: missing '='");
			goto error_label;
		}
		*eq = '\0';
		val = eq + 1;
		if ((a->opaque = t_tag_new(key, val)) == NULL)
			goto error_label;
		free(key);
		key = NULL;
		a->write = 1;
		a->apply = t_action_set;
		break;
	}

	return (a);
	/* NOTREACHED */
error_label:
	free(key);
	t_action_delete(a);
	return (NULL);
}


static void
t_action_delete(struct t_action *victim)
{

	if (victim == NULL)
		return;

	switch (victim->kind) {
	case T_ACTION_ADD: /* FALLTHROUGH */
	case T_ACTION_SET: /* FALLTHROUGH */
		t_tag_delete(victim->opaque);
		break;
	case T_ACTION_CLEAR: /* FALLTHROUGH */
	case T_ACTION_LOAD:
		free(victim->opaque);
		break;
	case T_ACTION_RENAME:
		t_rename_pattern_delete(victim->opaque);
		break;
	default:
		/* do nada */
		break;
	}
	free(victim);
}


static int
t_action_add(struct t_action *self, struct t_tune *tune)
{

	const struct t_tag *t;
	struct t_taglist *tlist = NULL;

	assert_not_null(self);
	assert(self->kind == T_ACTION_ADD);
	assert_not_null(tune);

	t = self->opaque;

	tlist = t_tune_tags(tune);
	if (tlist == NULL)
		goto error_label;

	if (t_taglist_insert(tlist, t->key, t->val) != 0)
		goto error_label;
	if (t_tune_set_tags(tune, tlist) != 0)
		goto error_label;

	t_taglist_delete(tlist);
	return (0);
	/* NOTREACHED */
error_label:
	t_taglist_delete(tlist);
	return (-1);
}


static int
t_action_backend(struct t_action *self, struct t_tune *tune)
{

	assert_not_null(self);
	assert_not_null(tune);
	assert(self->kind == T_ACTION_BACKEND);

	(void)printf("%s %s\n", t_tune_backend(tune)->libid, t_tune_path(tune));
	return (0);
}


static int
t_action_clear(struct t_action *self, struct t_tune *tune)
{
	const struct t_tag *t;
	struct t_taglist *tlist = NULL;
	struct t_taglist *ret;
	const char *clear_key;

	assert_not_null(self);
	assert(self->kind == T_ACTION_CLEAR);
	assert_not_null(tune);

	clear_key = self->opaque;
	ret = t_taglist_new();
	if (ret == NULL)
		goto error_label;

	if (clear_key != NULL) {
		if ((tlist = t_tune_tags(tune)) == NULL)
			goto error_label;
		TAILQ_FOREACH(t, tlist->tags, entries) {
			if (t_tag_keycmp(t->key, clear_key) != 0) {
				if (t_taglist_insert(ret, t->key, t->val) != 0)
					goto error_label;
			}
		}
	}
	if (t_tune_set_tags(tune, ret) != 0)
		goto error_label;

	t_taglist_delete(ret);
	t_taglist_delete(tlist);
	return (0);
	/* NOTREACHED */
error_label:
	t_taglist_delete(ret);
	t_taglist_delete(tlist);
	return (-1);
}


static int
t_action_edit(struct t_action *self, struct t_tune *tune)
{

	assert_not_null(self);
	assert(self->kind == T_ACTION_EDIT);
	assert_not_null(tune);

	return (t_edit(tune));
}


static int
t_action_load(struct t_action *self, struct t_tune *tune)
{

	assert_not_null(self);
	assert(self->kind == T_ACTION_LOAD);
	assert_not_null(tune);

	return (t_load(tune, self->opaque));
}


static int
t_action_print(struct t_action *self, struct t_tune *tune)
{
	char	*yaml;
	struct t_taglist *tlist;

	assert_not_null(self);
	assert(self->kind == T_ACTION_PRINT);
	assert_not_null(tune);

	tlist = t_tune_tags(tune);
	if (tlist == NULL)
		return (-1);
	yaml = t_tags2yaml(tlist, t_tune_path(tune));
	t_taglist_delete(tlist);
	tlist = NULL;
	if (yaml == NULL)
		return (-1);
	(void)printf("%s\n", yaml);
	free(yaml);
	return (0);
}


static int
t_action_rename(struct t_action *self, struct t_tune *tune)
{

	assert_not_null(self);
	assert(self->kind == T_ACTION_RENAME);
	assert_not_null(tune);

	return (t_rename(tune, self->opaque));
}


static int
t_action_set(struct t_action *self, struct t_tune *tune)
{
	struct t_tag *t, *t_tmp, *neo;
	struct t_taglist *tlist;
	int n, status;

	assert_not_null(self);
	assert(self->kind == T_ACTION_SET);
	assert_not_null(tune);

	t_tmp = self->opaque;
	neo = t_tag_new(t_tmp->key, t_tmp->val);
	if (neo == NULL)
		return (-1);
	t_tmp = NULL;

	tlist = t_tune_tags(tune);
	if (tlist == NULL) {
		t_tag_delete(neo);
		return (-1);
	}

	/* We want to "replace" existing tag(s) with a matching key. If there is
	   any, neo replace the first occurence found */
	n = 0;
	TAILQ_FOREACH_SAFE(t, tlist->tags, entries, t_tmp) {
		if (t_tag_keycmp(neo->key, t->key) == 0) {
			if (++n == 1)
				TAILQ_INSERT_BEFORE(t, neo, entries);
			TAILQ_REMOVE(tlist->tags, t, entries);
			t_tag_delete(t);
		}
	}
	if (n == 0) {
		/* there wasn't any tag matching the key, so we just add neo at
		   the end. */
		TAILQ_INSERT_TAIL(tlist->tags, neo, entries);
	}

	status = t_tune_set_tags(tune, tlist);
	t_taglist_delete(tlist);
	return (status == 0 ? 0 : -1);
}


/* used to search in the t_action_keywords array */
static int
t_action_token_cmp(const void *vstr, const void *vtoken)
{
	const char	*str;
	size_t		tlen, slen;
	int		cmp;
	const struct t_action_token *token;

	assert_not_null(vstr);
	assert_not_null(vtoken);

	token = vtoken;
	str   = vstr;
	slen = strlen(str);
	tlen = strlen(token->word);
	cmp = strncmp(str, token->word, tlen);

	/* we accept either slen == tlen or a finishing `:' */
	if (cmp == 0 && slen > tlen && str[tlen] != ':')
		cmp = str[tlen] - '\0';

	return (cmp);
}

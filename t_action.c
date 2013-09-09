/*
 * t_action.c
 *
 * tagutil actions.
 */

#include "t_config.h"
#include "t_backend.h"
#include "t_tag.h"
#include "t_action.h"

#include "t_yaml.h"
#include "t_renamer.h"

#include "t_lexer.h"
#include "t_parser.h"
#include "t_filter.h"


struct t_action_token {
	const char		*word;
	enum t_actionkind 	 kind;
	int			 argc;
};


/* keep this array sorted, as it is used with bsearch(3) */
static struct t_action_token t_action_keywords[] = {
	{ "add",	T_ACTION_ADD,		1 },
	{ "backend",	T_ACTION_BACKEND,	0 },
	{ "clear",	T_ACTION_CLEAR,		1 },
	{ "edit",	T_ACTION_EDIT,		0 },
	{ "filter",	T_ACTION_FILTER,	1 },
	{ "load",	T_ACTION_LOAD,		1 },
	{ "path",	T_ACTION_PATH,		0 },
	{ "print",	T_ACTION_PRINT,		0 },
	{ "rename",	T_ACTION_RENAME,	1 },
	{ "set",	T_ACTION_SET,		1 },
	{ "show",	T_ACTION_PRINT,		0 },
};


/*
 * create a new action.
 * return NULL on error and set errno to ENOMEM or EINVAL
 */
static struct t_action	*t_action_new(enum t_actionkind kind, char *arg);
/* free an action and all internal ressources */
static void		 t_action_delete(struct t_action *victim);

/* action methods */
static int	t_action_add(struct t_action *self, struct t_file *file);
static int	t_action_backend(struct t_action *self, struct t_file *file);
static int	t_action_clear(struct t_action *self, struct t_file *file);
static int	t_action_edit(struct t_action *self, struct t_file *file);
static int	t_action_load(struct t_action *self, struct t_file *file);
static int	t_action_print(struct t_action *self, struct t_file *file);
static int	t_action_path(struct t_action *self, struct t_file *file);
static int	t_action_rename(struct t_action *self, struct t_file *file);
static int	t_action_set(struct t_action *self, struct t_file *file);
static int	t_action_filter(struct t_action *self, struct t_file *file);
static int	t_action_reload(struct t_action *self, struct t_file *file);
static int	t_action_save(struct t_action *self, struct t_file *file);


/* used by bsearch(3) in the t_action_keywords array */
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


struct t_actionQ *
t_actionQ_new(int *argc_p, char ***argv_p, int *write_p)
{
	int write, argc;
	char **argv;
	struct t_action  *a;
	struct t_actionQ *aQ;

	assert_not_null(argc_p);
	assert_not_null(argv_p);

	argc = *argc_p;
	argv = *argv_p;
	aQ = malloc(sizeof(struct t_actionQ));
	if (aQ == NULL)
		goto error;
	TAILQ_INIT(aQ);

	while (argc > 0) {
		char	*arg;
		struct t_action_token *t;

		t = bsearch(*argv, t_action_keywords,
		    countof(t_action_keywords), sizeof(*t_action_keywords),
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
				goto error;
				/* NOTREACHED */
			}
			arg++; /* skip : */
		} else /* if t->argc > 1 */
			assert_fail();

		switch (t->kind) {
		case T_ACTION_FILTER:
			a = t_action_new(T_ACTION_SAVE, NULL);
			if (a == NULL)
				goto error;
			TAILQ_INSERT_TAIL(aQ, a, entries);
			/* FALLTHROUGH */
		case T_ACTION_ADD: /* FALLTHROUGH */
		case T_ACTION_CLEAR: /* FALLTHROUGH */
		case T_ACTION_EDIT: /* FALLTHROUGH */
		case T_ACTION_LOAD: /* FALLTHROUGH */
		case T_ACTION_SET: /* FALLTHROUGH */
		case T_ACTION_PRINT: /* FALLTHROUGH */
		case T_ACTION_BACKEND: /* FALLTHROUGH */
		case T_ACTION_PATH:
			a = t_action_new(t->kind, arg);
			if (a == NULL)
				goto error;
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		case T_ACTION_RENAME:
			a = t_action_new(T_ACTION_SAVE, NULL);
			if (a == NULL)
				goto error;
			TAILQ_INSERT_TAIL(aQ, a, entries);
			a = t_action_new(T_ACTION_RENAME, arg);
			if (a == NULL)
				goto error;
			TAILQ_INSERT_TAIL(aQ, a, entries);
			a = t_action_new(T_ACTION_RELOAD, NULL);
			if (a == NULL)
				goto error;
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		}
		argc--;
		argv++;
	}

	if (TAILQ_EMPTY(aQ)) {
		/* no action given, fallback to default which is show */
		a = t_action_new(T_ACTION_PRINT, NULL);
		if (a == NULL)
			goto error;
		TAILQ_INSERT_TAIL(aQ, a, entries);
	}

	/* check if write access is needed */
	write = 0;
	TAILQ_FOREACH(a, aQ, entries)
		write += a->write;
	if (write > 0) {
		a = t_action_new(T_ACTION_SAVE, NULL);
		if (a == NULL)
			goto error;
		TAILQ_INSERT_TAIL(aQ, a, entries);
	}

	if (write_p != NULL)
		*write_p = write;
	*argc_p = argc;
	*argv_p = argv;
	return (aQ);
	/* NOTREACHED */
error:
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


static struct t_action *
t_action_new(enum t_actionkind kind, char *arg)
{
	char *key, *val, *eq;
	struct t_action *a;
	struct t_taglist *tlist = NULL;

	a = calloc(1, sizeof(struct t_action));
	if (a == NULL)
		goto error;
	a->kind = kind;

	switch (a->kind) {
	case T_ACTION_ADD:
		assert_not_null(arg);
		if ((tlist = t_taglist_new()) == NULL)
			goto error;
		key = arg;
		eq = strchr(key, '=');
		if (eq == NULL) {
			warnc(errno = EINVAL, "add: missing '='");
			goto error;
		}
		*eq = '\0';
		val = eq + 1;
		if (t_taglist_insert(tlist, key, val) != 0)
			goto error;
		a->data  = tlist;
		a->write = 1;
		a->apply = t_action_add;
		break;
	case T_ACTION_BACKEND:
		a->apply = t_action_backend;
		break;
	case T_ACTION_CLEAR:
		assert_not_null(arg);
		if (arg[0] == '\0')
			tlist = NULL;
		else {
			if ((tlist = t_taglist_new()) == NULL)
				goto error;
			if ((t_taglist_insert(tlist, arg, "")) != 0)
				goto error;
		}
		a->data  = tlist;
		a->write = 1;
		a->apply = t_action_clear;
		break;
	case T_ACTION_EDIT:
		a->write = 1;
		a->apply = t_action_edit;
		break;
	case T_ACTION_LOAD:
		assert_not_null(arg);
		a->data  = arg;
		a->write = 1;
		a->apply = t_action_load;
		break;
	case T_ACTION_PRINT:
		a->apply = t_action_print;
		break;
	case T_ACTION_PATH:
		a->apply = t_action_path;
		break;
	case T_ACTION_RENAME:
		assert_not_null(arg);
		if (arg[0] == '\0') {
			warnc(errno = EINVAL, "empty rename pattern");
			goto error;
		}
		a->data  = t_rename_parse(arg);
		/* FIXME: error checking on t_rename_parse() */
		a->write = 1;
		a->apply = t_action_rename;
		break;
	case T_ACTION_SET:
		assert_not_null(arg);
		if ((tlist = t_taglist_new()) == NULL)
			goto error;
		key = arg;
		eq = strchr(key, '=');
		if (eq == NULL)
			warnc(errno = EINVAL, "set: missing '='");
		*eq = '\0';
		val = eq + 1;
		if ((t_taglist_insert(tlist, key, val)) != 0)
			goto error;
		a->data  = tlist;
		a->write = 1;
		a->apply = t_action_set;
		break;
	case T_ACTION_FILTER:
		assert_not_null(arg);
		a->data  = t_parse_filter(t_lexer_new(arg));
		/* FIXME: error checking on t_parse_filter & t_lexer_new */
		a->apply = t_action_filter;
		break;
	case T_ACTION_RELOAD:
		a->apply = t_action_reload;
		break;
	case T_ACTION_SAVE:
		a->write = 1;
		a->apply = t_action_save;
		break;
	default:
		assert_fail();
	}

	return (a);
	/* NOTREACHED */
error:
	t_taglist_delete(tlist);
	t_action_delete(a);
	return (NULL);
}


static void
t_action_delete(struct t_action *victim)
{
	int i;
	struct token **tknv;

	if (victim == NULL)
		return;

	switch (victim->kind) {
	case T_ACTION_ADD:	/* FALLTHROUGH */
	case T_ACTION_CLEAR:	/* FALLTHROUGH */
	case T_ACTION_SET:
		t_taglist_delete(victim->data);
		break;
	case T_ACTION_RENAME:
		tknv = victim->data;
		for (i = 0; tknv[i]; i++)
			free(tknv[i]);
		free(tknv);
		break;
	case T_ACTION_FILTER:
		t_ast_destroy(victim->data);
		break;
	default:
		/* do nada */
		break;
	}
	free(victim);
}


static int
t_action_add(struct t_action *self, struct t_file *file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_ADD);

	return (file->add(file, self->data));
}


static int
t_action_backend(struct t_action *self, struct t_file *file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_BACKEND);

	(void)printf("%s %s\n", file->libid, file->path);
	return (true);
}


static int
t_action_clear(struct t_action *self, struct t_file *file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_CLEAR);

	return (file->clear(file, self->data));
}


static int
t_action_edit(struct t_action *self, struct t_file *file)
{
	int	retval = true;
	char	*tmp_file, *yaml, *question;
	FILE	*stream;
	struct t_action *load;
	struct t_taglist *tlist;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_EDIT);

	tlist = file->get(file, NULL);
	if (tlist == NULL)
		return (false);
	yaml = t_tags2yaml(tlist, file->path);
	t_taglist_delete(tlist);
	if (yaml == NULL)
		return (false);

	(void)xasprintf(&question, "edit %s", file->path);
	if (t_yesno(question)) {
		(void)xasprintf(&tmp_file, "/tmp/%s-XXXXXX.yml", getprogname());
		if (mkstemps(tmp_file, 4) == -1)
			err(errno, "mkstemps");
		stream = fopen(tmp_file, "w");
		if (stream == NULL)
			err(errno, "fopen %s", tmp_file);
		(void)fprintf(stream, "%s", yaml);
		if (fclose(stream) != 0)
			err(errno, "fclose %s", tmp_file);
		if (t_user_edit(tmp_file)) {
			load = t_action_new(T_ACTION_LOAD, tmp_file);
			retval = load->apply(load, file);
			t_action_delete(load);
		}
		(void)unlink(tmp_file);
		freex(tmp_file);
	}

	free(question);
	free(yaml);
	return (retval);
}


static int
t_action_load(struct t_action *self, struct t_file *file)
{
	int	retval = true;
	FILE	*stream;
	const char *path;
	struct t_taglist *tlist;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_LOAD);

	path = self->data;
	if (strcmp(path, "-") == 0)
		stream = stdin;
	else {
		stream = fopen(path, "r");
		if (stream == NULL)
			err(errno, "fopen %s", path);
	}

	tlist = t_yaml2tags(file, stream);
	if (stream != stdin)
		(void)fclose(stream);
	if (tlist == NULL)
		return (false);
	retval = file->clear(file, NULL) &&
	    file->add(file, tlist) &&
	    file->save(file);
	t_taglist_delete(tlist);
	return (retval);
}


static int
t_action_print(struct t_action *self, struct t_file *file)
{
	char	*yaml;
	struct t_taglist *tlist;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_PRINT);

	tlist = file->get(file, NULL);
	if (tlist == NULL)
		return (false);
	yaml = t_tags2yaml(tlist, file->path);
	t_taglist_delete(tlist);
	if (yaml == NULL)
		return (false);
	(void)printf("%s\n", yaml);
	freex(yaml);
	return (true);
}


static int
t_action_path(struct t_action *self, struct t_file *file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_PATH);

	(void)printf("%s\n", file->path);
	return (true);
}


static int
t_action_rename(struct t_action *self, struct t_file *file)
{
	int	retval = true;
	char	*ext, *result, *fname, *question;
	const char	*dirn;
	struct t_token **tknv;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_RENAME);

	tknv = self->data;
	t_error_clear(file);

	ext = strrchr(file->path, '.');
	if (ext == NULL) {
		t_error_set(file, "can't find file extension for `%s'",
		    file->path);
		return (false);
	}
	ext++; /* skip dot */
	fname = t_rename_eval(file, tknv);
	if (fname == NULL)
		return (false);

	/* fname is now OK. store into result the full new path.  */
	dirn = t_dirname(file->path);
	if (dirn == NULL)
		err(errno, "dirname");
	/* add the directory to result if needed */
	if (strcmp(dirn, ".") != 0)
		(void)xasprintf(&result, "%s/%s.%s", dirn, fname, ext);
	else
		(void)xasprintf(&result, "%s.%s", fname, ext);
	freex(fname);

	/* ask user for confirmation and rename if user want to */
	if (strcmp(file->path, result) != 0) {
		(void)xasprintf(&question, "rename `%s' to `%s'",
		    file->path, result);
		if (t_yesno(question)) {
			retval = t_rename_safe(file, result);
		}
		freex(question);
	}
	freex(result);
	return (retval);
}


static int
t_action_set(struct t_action *self, struct t_file *file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_SET);

	return (file->clear(file, self->data) && file->add(file, self->data));
}


/* FIXME: error handling? */
static int
t_action_filter(struct t_action *self, struct t_file *file)
{
	const struct t_ast *ast;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_FILTER);
	assert_not_null(file);

	ast = self->data;
	return (t_filter_eval(file, ast));
}


static int
t_action_reload(struct t_action *self, struct t_file *file)
{
	struct t_file	tmp;
	struct t_file	*neo;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_RELOAD);
	assert(!file->dirty);

	neo = file->new(file->path);
	/* switch */
	memcpy(&tmp, file, sizeof(struct t_file));
	memcpy(file, neo,  sizeof(struct t_file));
	memcpy(neo,  &tmp, sizeof(struct t_file));
	/* now neo is in fact the "old" one */
	neo->destroy(neo);
	return (true);
}


static int
t_action_save(struct t_action *self, struct t_file *file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_SAVE);

	return (!file->dirty || file->save(file));
}

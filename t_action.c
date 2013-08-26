/*
 * @(#)t_action.c
 *
 * tagutil actions.
 */
#include "t_config.h"
#include "t_backend.h"
#include "t_tag.h"
#include "t_action.h"

#include "t_renamer.h"

#include "t_yaml.h"

#include "t_lexer.h"
#include "t_parser.h"
#include "t_filter.h"


struct t_action_token {
	const char		*word;
	enum t_actionkind 	kind;
	bool	need_arg;
};

/* keep this array sorted, as it is used with bsearch(3) */
static struct t_action_token t_action_keywords[] = {
	{ "add",	T_ACTION_ADD,		true  },
	{ "backend",	T_ACTION_SHOWBACKEND,	false },
	{ "clear",	T_ACTION_CLEAR,		true  },
	{ "edit",	T_ACTION_EDIT,		false },
	{ "filter",	T_ACTION_FILTER,	true  },
	{ "load",	T_ACTION_LOAD,		true  },
	{ "path",	T_ACTION_SHOWPATH,	false },
	{ "print",	T_ACTION_SHOW,		false },
	{ "rename",	T_ACTION_RENAME,	true  },
	{ "set",	T_ACTION_SET,		true  },
	{ "show",	T_ACTION_SHOW,		false },
};


/* action methods */
static bool	t_action_add(struct t_action *self, struct t_file *file);
static bool	t_action_backend(struct t_action *self, struct t_file *file);
static bool	t_action_clear(struct t_action *self, struct t_file *file);
static bool	t_action_edit(struct t_action *self, struct t_file *file);
static bool	t_action_load(struct t_action *self, struct t_file *file);
static bool	t_action_print(struct t_action *self, struct t_file *file);
static bool	t_action_showpath(struct t_action *self, struct t_file *file);
static bool	t_action_rename(struct t_action *self, struct t_file *file);
static bool	t_action_set(struct t_action *self, struct t_file *file);
static bool	t_action_filter(struct t_action *self, struct t_file *file);
static bool	t_action_reload(struct t_action *self, struct t_file *file);
static bool	t_action_saveifdirty(struct t_action *self, struct t_file *file);


/*
 * create a single action.
 */
static struct t_action *	t_action_new(enum t_actionkind kind, char *arg);

/*
 * destroy a single action.
 */
_t__nonnull(1)
static void	t_action_destroy(struct t_action *a);


/*
 * show usage and exit.
 */
void
usage(void)
{
	const struct t_backendQ *bQ = t_all_backends();
	const struct t_backend	*b;

	(void)fprintf(stderr, "tagutil v"T_TAGUTIL_VERSION "\n\n");
	(void)fprintf(stderr, "usage: %s [OPTION]... [ACTION:ARG]... [FILE]...\n",
	    getprogname());
	(void)fprintf(stderr, "Modify or display music file's tag.\n");
	(void)fprintf(stderr, "\n");

	(void)fprintf(stderr, "Options:\n");
	(void)fprintf(stderr, "  -h  show this help\n");
	(void)fprintf(stderr, "  -d  create directory on rename if needed\n");
	(void)fprintf(stderr, "  -Y  answer yes to all questions\n");
	(void)fprintf(stderr, "  -N  answer no  to all questions\n");
	(void)fprintf(stderr, "\n");

	(void)fprintf(stderr, "Actions:\n");
	(void)fprintf(stderr, "  add:TAG=VALUE    add a TAG=VALUE pair\n");
	(void)fprintf(stderr, "  backend          print backend used\n");
	(void)fprintf(stderr, "  clear:TAG        clear all tag TAG\n");
	(void)fprintf(stderr, "  edit             prompt for editing\n");
	(void)fprintf(stderr, "  filter:FILTER    use only files matching "
	    "FILTER for next(s) action(s)\n");
	(void)fprintf(stderr, "  load:PATH        load PATH yaml tag file\n");
	(void)fprintf(stderr, "  path             print only filename's path\n");
	(void)fprintf(stderr, "  print (or show)  print tags (default action)\n");
	(void)fprintf(stderr, "  rename:PATTERN   rename to PATTERN\n");
	(void)fprintf(stderr, "  set:TAG=VALUE    set TAG to VALUE\n");
	(void)fprintf(stderr, "\n");

	(void)fprintf(stderr, "Backend:\n");
	TAILQ_FOREACH(b, bQ, entries)
		(void)fprintf(stderr, "  %10s: %s\n", b->libid, b->desc);
	(void)fprintf(stderr, "\n");

	exit(EXIT_SUCCESS);
}


static int t_action_token_cmp(const void *_str, const void *_token)
{
	const char	*str;
	size_t		tlen, slen;
	int		cmp;
	const struct t_action_token *token;

	assert_not_null(_str);
	assert_not_null(_token);

	token = _token;
	str   = _str;
	slen = strlen(str);
	tlen = strlen(token->word);
	cmp = strncmp(str, token->word, tlen);

	/* we accept either slen == tlen or a finishing `:' */
	if (cmp == 0 && slen > tlen && str[tlen] != ':')
		cmp = str[tlen] - '\0';

	return (cmp);
}


struct t_actionQ *
t_actionQ_create(int *argcp, char ***argvp, bool *writep)
{
	bool	write = false; /* at least one action want to write */
	int 	argc;
	char 	**argv;
	struct t_action		*a;
	struct t_actionQ	*aQ;

	assert_not_null(argcp);
	assert_not_null(argvp);

	argc = *argcp;
	argv = *argvp;
	aQ = xmalloc(sizeof(struct t_actionQ));
	TAILQ_INIT(aQ);

	while (argc > 0) {
		char	*arg;
		struct t_action_token *t;

		t = bsearch(*argv, t_action_keywords, countof(t_action_keywords),
		    sizeof(*t_action_keywords), t_action_token_cmp);
		if (t == NULL) {
			/* it doesn't look like an option */
			break;
		}
		if (t->need_arg) {
			arg = strchr(*argv, ':');
			if (arg == NULL) {
				warnx("option requires an argument -- %s", t->word);
				usage();
				/* NOTREACHED */
			}
			arg++; /* skip : */
		} else
			arg = NULL;

		switch (t->kind) {
		case T_ACTION_FILTER:
			a = t_action_new(T_ACTION_SAVE_IF_DIRTY, NULL);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			/* FALLTHROUGH */
		case T_ACTION_ADD: /* FALLTHROUGH */
		case T_ACTION_CLEAR: /* FALLTHROUGH */
		case T_ACTION_EDIT: /* FALLTHROUGH */
		case T_ACTION_LOAD: /* FALLTHROUGH */
		case T_ACTION_SET: /* FALLTHROUGH */
		case T_ACTION_SHOW: /* FALLTHROUGH */
		case T_ACTION_SHOWBACKEND: /* FALLTHROUGH */
		case T_ACTION_SHOWPATH:
			a = t_action_new(t->kind, arg);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		case T_ACTION_RENAME:
			a = t_action_new(T_ACTION_SAVE_IF_DIRTY, NULL);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			a = t_action_new(T_ACTION_RENAME, arg);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			a = t_action_new(T_ACTION_RELOAD, NULL);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		default:
			assert_fail();
		}
		write = (write || a->write);
		argc--;
		argv++;
	}

	if (TAILQ_EMPTY(aQ)) {
		/* no action given, fallback to default which is show */
		a = t_action_new(T_ACTION_SHOW, NULL);
		TAILQ_INSERT_TAIL(aQ, a, entries);
	}

	/* check if write access is needed */
	if (write) {
		a = t_action_new(T_ACTION_SAVE_IF_DIRTY, NULL);
		TAILQ_INSERT_TAIL(aQ, a, entries);
	}
	if (writep != NULL)
		*writep = write;

	*argcp = argc;
	*argvp = argv;
	return (aQ);
}


void
t_actionQ_destroy(struct t_actionQ *aQ)
{
	struct t_action	*victim, *next;

	assert_not_null(aQ);

	victim = TAILQ_FIRST(aQ);
	while (victim != NULL) {
		next = TAILQ_NEXT(victim, entries);
		t_action_destroy(victim);
		victim = next;
	}
	free(aQ);
}


static struct t_action *
t_action_new(enum t_actionkind kind, char *arg)
{
	char		*key, *value;
	struct t_action		*a;
	struct t_taglist	*T;

	a = xcalloc(1, sizeof(struct t_action));
	a->kind = kind;
	switch (a->kind) {
	case T_ACTION_ADD:
		assert_not_null(arg);
		T = t_taglist_new();
		key = arg;
		value = strchr(key, '=');
		if (value == NULL)
			errx(EINVAL, "`%s': invalid `add' argument: `=' is missing.", key);
		value += 1;
		t_taglist_insert(T, key, value);
		a->data  = T;
		a->write = true;
		a->apply = t_action_add;
		break;
	case T_ACTION_SHOWBACKEND:
		a->apply = t_action_backend;
		break;
	case T_ACTION_CLEAR:
		assert_not_null(arg);
		T = t_taglist_new();
		t_taglist_insert(T, arg, "");
		a->data  = T;
		a->write = true;
		a->apply = t_action_clear;
		break;
	case T_ACTION_EDIT:
		a->write = true;
		a->apply = t_action_edit;
		break;
	case T_ACTION_LOAD:
		assert_not_null(arg);
		a->data  = arg;
		a->write = true;
		a->apply = t_action_load;
		break;
	case T_ACTION_SHOW:
		a->apply = t_action_print;
		break;
	case T_ACTION_SHOWPATH:
		a->apply = t_action_showpath;
		break;
	case T_ACTION_RENAME:
		assert_not_null(arg);
		if (t_strempty(arg))
			errx(EINVAL, "empty rename pattern");
		a->data  = t_rename_parse(arg);
		a->write = true;
		a->apply = t_action_rename;
		break;
	case T_ACTION_SET:
		assert_not_null(arg);
		T = t_taglist_new();
		key = arg;
		value = strchr(key, '=');
		if (value == NULL)
			errx(EINVAL, "`%s': invalid `set' argument, `=' is missing. ", key);
		value += 1;
		t_taglist_insert(T, key, value);
		a->data  = T;
		a->write = true;
		a->apply = t_action_set;
		break;
	case T_ACTION_FILTER:
		assert_not_null(arg);
		a->data  = t_parse_filter(t_lexer_new(arg));
		a->apply = t_action_filter;
		break;
	case T_ACTION_RELOAD:
		a->apply = t_action_reload;
		break;
	case T_ACTION_SAVE_IF_DIRTY:
		a->write = false; /* writting is not needed after a T_ACTION_SAVE_IF_DIRTY */
		a->apply = t_action_saveifdirty;
		break;
	default:
		assert_fail();
	}
	return (a);
}


static void
t_action_destroy(struct t_action *a)
{
	int	i;
	struct token	**tknv;

	assert_not_null(a);

	switch (a->kind) {
	case T_ACTION_ADD:	/* FALLTHROUGH */
	case T_ACTION_CLEAR:	/* FALLTHROUGH */
	case T_ACTION_SET:
		t_taglist_destroy(a->data);
		break;
	case T_ACTION_RENAME:
		tknv = a->data;
		for (i = 0; tknv[i]; i++)
			free(tknv[i]);
		free(tknv);
		break;
	case T_ACTION_FILTER:
		t_ast_destroy(a->data);
		break;
	default:
		/* do nada */
		break;
	}
	free(a);
}


static bool
t_action_add(struct t_action *self, struct t_file *file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_ADD);

	return (file->add(file, self->data));
}


static bool
t_action_backend(struct t_action *self, struct t_file *file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_SHOWBACKEND);

	(void)printf("%s: %s\n", file->path, file->libid);
	return (true);
}


static bool
t_action_clear(struct t_action *self, struct t_file *file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_CLEAR);

	return (file->clear(file, self->data));
}


static bool
t_action_edit(struct t_action *self, struct t_file *file)
{
	bool	retval = true;
	char	*tmp_file, *yaml;
	FILE	*stream;
	struct t_action	*load;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_EDIT);

	yaml = t_tags2yaml(file);
	if (yaml == NULL)
		return (false);

	/* FIXME: just yesno(question); */
	(void)printf("edit ");
	if (t_yesno(file->path)) {
    		(void)xasprintf(&tmp_file, "/tmp/%s-XXXXXX.yml", getprogname());
		if (mkstemp(tmp_file) == -1)
			err(errno, "mkstemp");
		stream = xfopen(tmp_file, "w");
		(void)fprintf(stream, "%s", yaml);
		xfclose(stream);
		if (t_user_edit(tmp_file)) {
			load = t_action_new(T_ACTION_LOAD, tmp_file);
			retval = load->apply(load, file);
			t_action_destroy(load);
		}
		xunlink(tmp_file);
		freex(tmp_file);
	}
	freex(yaml);
	return (retval);
}


static bool
t_action_load(struct t_action *self, struct t_file *file)
{
	bool	retval = true;
	FILE	*stream;
	const char *path;
	struct t_taglist *T;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_LOAD);

	path = self->data;
	if (strcmp(path, "-") == 0)
		stream = stdin;
	else
		stream = xfopen(path, "r");

	T = t_yaml2tags(file, stream);
	if (stream != stdin)
		xfclose(stream);
	if (T == NULL)
		return (false);
	retval = file->clear(file, NULL) &&
	    file->add(file, T) &&
	    file->save(file);
	t_taglist_destroy(T);
	return (retval);
}


static bool
t_action_print(struct t_action *self, struct t_file *file)
{
	char	*yaml;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_SHOW);

	yaml = t_tags2yaml(file);
	if (yaml == NULL)
		return (false);
	(void)printf("%s\n", yaml);
	freex(yaml);
	return (true);
}


static bool
t_action_showpath(struct t_action *self, struct t_file *file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_SHOWPATH);

	(void)printf("%s\n", file->path);
	return (true);
}


static bool
t_action_rename(struct t_action *self, struct t_file *file)
{
	bool	retval = true;
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
		if (t_yesno(question))
			retval = t_rename_safe(file, result);
		freex(question);
	}
	freex(result);
	return (retval);
}


static bool
t_action_set(struct t_action *self, struct t_file *file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_SET);

	return (file->clear(file, self->data) && file->add(file, self->data));
}


/* FIXME: error handling? */
static bool
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


static bool
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


static bool
t_action_saveifdirty(struct t_action *self, struct t_file *file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ACTION_SAVE_IF_DIRTY);

	return (!file->dirty || file->save(file));
}

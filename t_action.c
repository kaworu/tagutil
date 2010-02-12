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


#define	GETOPT_STRING	"dhNY"

bool	dflag = false; /* create directory with rename */
bool	Nflag = false; /* answer no to all questions */
bool	Yflag = false; /* answer yes to all questions */


/* actions */
static bool	t_action_add(struct t_action *restrict self, struct t_file *restrict file);
static bool	t_action_backend(struct t_action *restrict self, struct t_file *restrict file);
static bool	t_action_clear(struct t_action *restrict self, struct t_file *restrict file);
static bool	t_action_edit(struct t_action *restrict self, struct t_file *restrict file);
static bool	t_action_load(struct t_action *restrict self, struct t_file *restrict file);
static bool	t_action_print(struct t_action *restrict self, struct t_file *restrict file);
static bool	t_action_showpath(struct t_action *restrict self, struct t_file *restrict file);
static bool	t_action_rename(struct t_action *restrict self, struct t_file *restrict file);
static bool	t_action_set(struct t_action *restrict self, struct t_file *restrict file);
static bool	t_action_filter(struct t_action *restrict self, struct t_file *restrict file);
static bool	t_action_reload(struct t_action *restrict self, struct t_file *restrict file);
static bool	t_action_saveifdirty(struct t_action *restrict self, struct t_file *restrict file);


void
usage(void)
{
	const struct t_backendQ *bQ = t_get_backend();
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


struct t_action_token {
	const char		*kstr;
	enum t_actionkind 	kind;
	bool	need_arg;
};

static struct t_action_token t_action_keywords[] = {
	{ "add",	T_ADD,		true  },
	{ "backend",	T_SHOWBACKEND,	false },
	{ "clear",	T_CLEAR,	true  },
	{ "edit",	T_EDIT,		false },
	{ "filter",	T_FILTER,	true  },
	{ "load",	T_LOAD,		true  },
	{ "path",	T_SHOWPATH,	false },
	{ "print",	T_SHOW,		false },
	{ "rename",	T_RENAME,	true  },
	{ "set",	T_SET,		true  },
	{ "show",	T_SHOW,		false },
};

static int t_action_token_cmp(const void *k, const void *t)
{
	const struct t_action_token *token;

	assert_not_null(k);
	assert_not_null(t);

	token = t;
	return (strncmp(k, token->kstr, strlen(token->kstr)));
}


struct t_actionQ *
t_actionQ_create(int *argcp, char ***argvp)
{
	int 	argc;
	char 	**argv;
	char	ch;
	struct t_action		*a;
	struct t_actionQ	*aQ;

	assert_not_null(argcp);
	assert_not_null(argvp);

	argc = *argcp;
	argv = *argvp;
	aQ = xmalloc(sizeof(struct t_actionQ));
	TAILQ_INIT(aQ);

	while ((ch = getopt(argc, argv, GETOPT_STRING)) != -1) {
		switch ((char)ch) {
		case 'd':
			dflag = true;
			break;
		case 'N':
			if (Yflag)
				errx(EINVAL, "cannot set both -Y and -N");
			Nflag = true;
			break;
		case 'Y':
			if (Nflag)
				errx(EINVAL, "cannot set both -Y and -N");
			Yflag = true;
			break;
		case 'h': /* FALLTHROUGH */
		case '?': /* FALLTHROUGH */
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	while (argc > 0) {
		char	*arg;
		struct t_action_token *t;

		t = bsearch(*argv, t_action_keywords, countof(t_action_keywords),
		    sizeof(*t_action_keywords), t_action_token_cmp);
		if (t == NULL)
			break;
		arg = strchr(*argv, ':');
		if (t->need_arg && arg == NULL) {
			warnx(EINVAL, "option requires an argument -- %s",
			    t->kstr);
			usage();
			/* NOTREACHED */
		}
		arg++; /* skip : */

		switch (t->kind) {
		case T_ADD:
			a = t_action_new(T_ADD, arg);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		case T_CLEAR:
			a = t_action_new(T_CLEAR, arg);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		case T_LOAD:
			a = t_action_new(T_LOAD, arg);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		case T_RENAME:
			a = t_action_new(T_SAVE_IF_DIRTY, NULL);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			a = t_action_new(T_RENAME, arg);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			a = t_action_new(T_RELOAD, NULL);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		case T_SET:
			a = t_action_new(T_SET, arg);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		case T_FILTER:
			a = t_action_new(T_SAVE_IF_DIRTY, NULL);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			a = t_action_new(T_FILTER, arg);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		case T_SHOWBACKEND:
			a = t_action_new(T_SHOWBACKEND, NULL);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		case T_EDIT:
			a = t_action_new(T_EDIT, NULL);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		case T_SHOW:
			a = t_action_new(T_SHOW, NULL);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		case T_SHOWPATH:
			a = t_action_new(T_SHOWPATH, NULL);
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		default:
			assert_fail();
		}
		argc--;
		argv++;
	}

	*argcp = argc;
	*argvp = argv;
	return (aQ);
}


void
t_actionQ_destroy(struct t_actionQ *restrict aQ)
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


struct t_action *
t_action_new(enum t_actionkind kind, char *arg)
{
	char	*key, *value;
	struct t_action		*a;
	struct t_taglist	*T;

	a = xcalloc(1, sizeof(struct t_action));
	a->kind = kind;
	switch (a->kind) {
	case T_ADD:
		assert_not_null(arg);
		T = t_taglist_new();
		key = arg;
		value = strchr(key, '=');
		if (value == NULL)
			errx(EINVAL, "`%s': invalid -a argument (missing =)", key);
		*value = '\0';
		value += 1;
		t_taglist_insert(T, key, value);
		a->data  = T;
		a->rw    = true;
		a->apply = t_action_add;
		break;
	case T_SHOWBACKEND:
		a->rw    = false;
		a->apply = t_action_backend;
		break;
	case T_CLEAR:
		assert_not_null(arg);
		T = t_taglist_new();
		t_taglist_insert(T, arg, "");
		a->data  = T;
		a->rw    = true;
		a->apply = t_action_clear;
		break;
	case T_EDIT:
		a->rw    = true;
		a->apply = t_action_edit;
		break;
	case T_LOAD:
		assert_not_null(arg);
		a->data  = arg;
		a->rw    = true;
		a->apply = t_action_load;
		break;
	case T_SHOW:
		a->rw    = false;
		a->apply = t_action_print;
		break;
	case T_SHOWPATH:
		a->rw    = false;
		a->apply = t_action_showpath;
		break;
	case T_RENAME:
		assert_not_null(arg);
		if (t_strempty(arg))
			errx(EINVAL, "empty rename pattern");
		a->data  = t_rename_parse(arg);
		a->rw    = true;
		a->apply = t_action_rename;
		break;
	case T_SET:
		assert_not_null(arg);
		T = t_taglist_new();
		key = arg;
		value = strchr(key, '=');
		if (value == NULL)
			errx(EINVAL, "`%s': invalid -s argument (missing =)", key);
		*value = '\0';
		value += 1;
		t_taglist_insert(T, key, value);
		a->data  = T;
		a->rw    = true;
		a->apply = t_action_set;
		break;
	case T_FILTER:
		assert_not_null(arg);
		a->data  = t_parse_filter(t_lexer_new(arg));
		a->rw    = false;
		a->apply = t_action_filter;
		break;
	case T_RELOAD:
		a->rw    = false;
		a->apply = t_action_reload;
		break;
	case T_SAVE_IF_DIRTY:
		a->rw    = false; /* yes, we lie, we're bad */
		a->apply = t_action_saveifdirty;
		break;
	default:
		assert_fail();
	}
	return (a);
}


void
t_action_destroy(struct t_action *a)
{
	int	i;
	struct token	**tknv;

	assert_not_null(a);

	switch (a->kind) {
	case T_ADD:	/* FALLTHROUGH */
	case T_CLEAR:	/* FALLTHROUGH */
	case T_SET:
		t_taglist_destroy(a->data);
		break;
	case T_RENAME:
		tknv = a->data;
		for (i = 0; tknv[i]; i++)
			free(tknv[i]);
		free(tknv);
		break;
	case T_FILTER:
		t_ast_destroy(a->data);
		break;
	default:
		/* do nada */
		break;
	}
	free(a);
}


static bool
t_action_add(struct t_action *restrict self, struct t_file *restrict file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_ADD);

	return (file->add(file, self->data));
}


static bool
t_action_backend(struct t_action *restrict self, struct t_file *restrict file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_SHOWBACKEND);

	(void)printf("%s: %s\n", file->path, file->libid);
	return (true);
}


static bool
t_action_clear(struct t_action *restrict self, struct t_file *restrict file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_CLEAR);

	return (file->clear(file, self->data));
}


static bool
t_action_edit(struct t_action *restrict self, struct t_file *restrict file)
{
	bool	retval = true;
	char	*tmp_file, *yaml, *editor;
	FILE	*stream;
	struct t_action	*load;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_EDIT);

	yaml = t_tags2yaml(file);
	if (yaml == NULL)
		return (false);

	(void)printf("edit ");
	if (t_yesno(file->path)) {
		tmp_file = t_mkstemp("/tmp");
		stream = xfopen(tmp_file, "w");
		(void)fprintf(stream, "%s", yaml);
		editor = getenv("EDITOR");
		if (editor != NULL && strcmp(editor, "vim") == 0)
			(void)fprintf(stream, "\n# vim:filetype=yaml");
		xfclose(stream);
		if (t_user_edit(tmp_file)) {
			load = t_action_new(T_LOAD, tmp_file);
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
t_action_load(struct t_action *restrict self, struct t_file *restrict file)
{
	bool	retval = true;
	FILE	*stream;
	const char *path;
	struct t_taglist *T;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_LOAD);

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
t_action_print(struct t_action *restrict self, struct t_file *restrict file)
{
	char	*yaml;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_SHOW);

	yaml = t_tags2yaml(file);
	if (yaml == NULL)
		return (false);
	(void)printf("%s\n", yaml);
	freex(yaml);
	return (true);
}


static bool
t_action_showpath(struct t_action *restrict self, struct t_file *restrict file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_SHOWPATH);

	(void)printf("%s\n", file->path);
	return (true);
}


static bool
t_action_rename(struct t_action *restrict self, struct t_file *restrict file)
{
	bool	retval = true;
	char	*ext, *result, *dirn, *fname, *question;
	struct t_token **tknv;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_RENAME);

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
	freex(dirn);
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
t_action_set(struct t_action *restrict self, struct t_file *restrict file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_SET);

	return (file->clear(file, self->data) && file->add(file, self->data));
}


/* FIXME: error handling? */
static bool
t_action_filter(struct t_action *restrict self, struct t_file *restrict file)
{
	const struct t_ast *ast;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_FILTER);
	assert_not_null(file);

	ast = self->data;
	return (t_filter_eval(file, ast));
}


static bool
t_action_reload(struct t_action *restrict self, struct t_file *restrict file)
{
	struct t_file	tmp;
	struct t_file	*neo;

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_RELOAD);
	assert(!file->dirty);

	neo = file->new(file->path);
	/* switch */
	memcpy(&tmp, file, sizeof(struct t_file));
	memcpy(file, neo,  sizeof(struct t_file));
	memcpy(neo,  &tmp, sizeof(struct t_file));
	neo->destroy(neo);
	return (true);
}




static bool
t_action_saveifdirty(struct t_action *restrict self, struct t_file *restrict file)
{

	assert_not_null(self);
	assert_not_null(file);
	assert(self->kind == T_SAVE_IF_DIRTY);

	return (!file->dirty || file->save(file));
}

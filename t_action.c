/*
 * @(#)t_action.c
 *
 * tagutil actions.
 */
#include <stdio.h>
#include <stdlib.h>

#include "t_config.h"
#include "t_backend.h"
#include "t_tag.h"
#include "t_action.h"

#include "t_renamer.h"

#include "t_lexer.h"
#include "t_parser.h"
#include "t_filter.h"


bool	dflag = false; /* create directory with rename */
bool	Nflag = false; /* answer no to all questions */
bool	Yflag = false; /* answer yes to all questions */


/* actions */
bool	t_action_add(struct t_action *restrict self, struct t_file **filep);
bool	t_action_backend(struct t_action *restrict self, struct t_file **filep);
bool	t_action_clear(struct t_action *restrict self, struct t_file **filep);
bool	t_action_edit(struct t_action *restrict self, struct t_file **filep);
bool	t_action_load(struct t_action *restrict self, struct t_file **filep);
bool	t_action_print(struct t_action *restrict self, struct t_file **filep);
bool	t_action_showpath(struct t_action *restrict self, struct t_file **filep);
bool	t_action_rename(struct t_action *restrict self, struct t_file **filep);
bool	t_action_set(struct t_action *restrict self, struct t_file **filep);
bool	t_action_filter(struct t_action *restrict self, struct t_file **filep);
bool	t_action_saveifdirty(struct t_action *restrict self, struct t_file **filep);


void
usage(void)
{
	const struct t_backendQ *bQ = t_get_backend();
	const struct t_backend	*b;

	(void)fprintf(stderr, "tagutil v"T_TAGUTIL_VERSION "\n\n");
	(void)fprintf(stderr, "usage: %s [OPTION]... [FILE]...\n", getprogname());
	(void)fprintf(stderr, "Modify or display music file's tag.\n");
	(void)fprintf(stderr, "\n");
	(void)fprintf(stderr, "Backend:\n");

	TAILQ_FOREACH(b, bQ, next)
		(void)fprintf(stderr, "  %10s: %s\n", b->libid, b->desc);
	(void)fprintf(stderr, "\n");
	(void)fprintf(stderr, "Options:\n");
	(void)fprintf(stderr, "  -h              show this help\n");
	(void)fprintf(stderr, "  -b              print backend used\n");
	(void)fprintf(stderr, "  -Y              answer yes to all questions\n");
	(void)fprintf(stderr, "  -N              answer no  to all questions\n");
	(void)fprintf(stderr, "  -a TAG=VALUE    add a TAG=VALUE pair\n");
	(void)fprintf(stderr, "  -c TAG          clear all tag TAG\n");
	(void)fprintf(stderr, "  -e              prompt for editing\n");
	(void)fprintf(stderr, "  -f PATH         load PATH yaml tag file\n");
	(void)fprintf(stderr, "  -p              print tags (default action)\n");
	(void)fprintf(stderr, "  -P              print only filename\n");
	(void)fprintf(stderr, "  -r [-d] PATTERN rename files with the given PATTERN\n");
	(void)fprintf(stderr, "  -s TAG=VALUE    set TAG to VALUE\n");
	(void)fprintf(stderr, "  -x FILTER       use only files matching FILTER for next(s) action(s)\n");
	(void)fprintf(stderr, "\n");

	exit(EXIT_SUCCESS);
}


struct t_actionQ *
t_actionQ_create(int *argcp, char ***argvp)
{
	int 	argc;
	char 	**argv;
	char	ch;
	struct t_actionQ	*aQ;

	assert_not_null(argcp);
	assert_not_null(argvp);

	argc = *argcp;
	argv = *argvp;
	aQ = xmalloc(sizeof(struct t_actionQ));
	TAILQ_INIT(aQ);

	while ((ch = getopt(argc, argv, GETOPT_STRING)) != -1) {
	switch ((char)ch) {
	case 'a':
		TAILQ_INSERT_TAIL(aQ, t_action_new(T_ADD, optarg), next);
		break;
	case 'b':
		TAILQ_INSERT_TAIL(aQ, t_action_new(T_SHOWBACKEND, NULL), next);
		break;
	case 'c':
		TAILQ_INSERT_TAIL(aQ, t_action_new(T_CLEAR, optarg), next);
		break;
	case 'd':
		dflag = true;
		break;
	case 'e':
		TAILQ_INSERT_TAIL(aQ, t_action_new(T_EDIT, NULL), next);
		break;
	case 'f':
		TAILQ_INSERT_TAIL(aQ, t_action_new(T_LOAD, optarg), next);
		break;
	case 'p':
		TAILQ_INSERT_TAIL(aQ, t_action_new(T_SHOW, NULL), next);
		break;
	case 'P':
		TAILQ_INSERT_TAIL(aQ, t_action_new(T_SHOWPATH, NULL), next);
		break;
	case 'r':
		TAILQ_INSERT_TAIL(aQ, t_action_new(T_RENAME, optarg), next);
		break;
	case 's':
		TAILQ_INSERT_TAIL(aQ, t_action_new(T_SET, optarg), next);
		break;
	case 'x':
		TAILQ_INSERT_TAIL(aQ, t_action_new(T_FILTER, optarg), next);
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

	*argcp = argc - optind;
	*argvp = argv + optind;
	return (aQ);
}


void
t_actionQ_destroy(struct t_actionQ *restrict aQ)
{
	struct t_action	*victim, *next;

	assert_not_null(aQ);

	victim = TAILQ_FIRST(aQ);
	while (victim != NULL) {
		next = TAILQ_NEXT(victim, next);
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
		/* FIXME: add a T_SAVE_IF_DIRTY before and T_RELOAD after */
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
		key = optarg;
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
		/* FIXME: add a T_SAVE_IF_DIRTY before ~ */
		a->apply = t_action_filter;
		break;
	case T_SAVE_IF_DIRTY:
		a->rw    = true;
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
	}
	free(a);
}


bool
t_action_add(struct t_action *restrict self, struct t_file **filep)
{
	struct t_file *file;

	assert_not_null(self);
	assert_not_null(filep);
	assert_not_null(*filep);
	assert(self->kind == T_ADD);

	file = *filep;

	return (file->add(file, self->data));
}


bool
t_action_backend(struct t_action *restrict self, struct t_file **filep)
{
	struct t_file *file;

	assert_not_null(self);
	assert_not_null(filep);
	assert_not_null(*filep);
	assert(self->kind == T_SHOWBACKEND);

	file = *filep;

	(void)printf("%s: %s\n", file->path, file->libid);
	return (true);
}


bool
t_action_clear(struct t_action *restrict self, struct t_file **filep)
{
	struct t_file *file;

	assert_not_null(self);
	assert_not_null(filep);
	assert_not_null(*filep);
	assert(self->kind == T_CLEAR);

	file = *filep;

	return (file->clear(file, self->data));
}


bool
t_action_edit(struct t_action *restrict self, struct t_file **filep)
{
	struct t_file *file;

	assert_not_null(self);
	assert_not_null(filep);
	assert_not_null(*filep);
	assert(self->kind == T_EDIT);

	file = *filep;

	return (tagutil_edit(file));
}


bool
t_action_load(struct t_action *restrict self, struct t_file **filep)
{
	struct t_file *file;

	assert_not_null(self);
	assert_not_null(filep);
	assert_not_null(*filep);
	assert(self->kind == T_LOAD);

	file = *filep;

	return (tagutil_load(file, self->data));
}


bool
t_action_print(struct t_action *restrict self, struct t_file **filep)
{
	struct t_file *file;

	assert_not_null(self);
	assert_not_null(filep);
	assert_not_null(*filep);
	assert(self->kind == T_SHOW);

	file = *filep;

	return (tagutil_print(file));
}


bool
t_action_showpath(struct t_action *restrict self, struct t_file **filep)
{
	struct t_file *file;

	assert_not_null(self);
	assert_not_null(filep);
	assert_not_null(*filep);
	assert(self->kind == T_SHOWPATH);

	file = *filep;

	(void)printf("%s\n", file->path);
	return (true);
}


bool
t_action_rename(struct t_action *restrict self, struct t_file **filep)
{
	struct t_file *file;

	assert_not_null(self);
	assert_not_null(filep);
	assert_not_null(*filep);
	assert(self->kind == T_RENAME);

	file = *filep;

	return (tagutil_rename(file, self->data));
}


bool
t_action_set(struct t_action *restrict self, struct t_file **filep)
{
	struct t_file *file;

	assert_not_null(self);
	assert_not_null(filep);
	assert_not_null(*filep);
	assert(self->kind == T_SET);

	file = *filep;
	return (file->clear(file, self->data) && file->add(file, self->data));
}


bool
t_action_filter(struct t_action *restrict self, struct t_file **filep)
{
	struct t_file *file;

	assert_not_null(self);
	assert_not_null(filep);
	assert_not_null(*filep);
	assert(self->kind == T_FILTER);

	file = *filep;

	return (tagutil_filter(file, self->data)); /* FIXME: error handling? */
}


bool
t_action_saveifdirty(struct t_action *restrict self, struct t_file **filep)
{
	struct t_file *file;

	assert_not_null(self);
	assert_not_null(filep);
	assert_not_null(*filep);
	assert(self->kind == T_SAVE_IF_DIRTY);

	file = *filep;
	return (!file->dirty || file->save(file));
}

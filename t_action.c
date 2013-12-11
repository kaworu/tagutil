/*
 * t_action.c
 *
 * tagutil actions.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "t_config.h"
#include "t_action.h"

#include "t_backend.h"
#include "t_taglist.h"
#include "t_tune.h"
#include "t_yaml.h"
#include "t_renamer.h"
#include "t_lexer.h"


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
/*
 * print the given question, and read user's input. input should match
 * y|yes|n|no.  t_yesno() loops until a valid response is given and then return
 * 1 if the response match y|yes, 0 if it match n|no.
 * Honor Yflag and Nflag.
 */
static int	t_yesno(const char *question);


/* action methods */
static int	t_action_add(struct t_action *self, struct t_tune *tune);
static int	t_action_backend(struct t_action *self, struct t_tune *tune);
static int	t_action_clear(struct t_action *self, struct t_tune *tune);
static int	t_action_edit(struct t_action *self, struct t_tune *tune);
static int	t_action_load(struct t_action *self, struct t_tune *tune);
static int	t_action_print(struct t_action *self, struct t_tune *tune);
static int	t_action_rename(struct t_action *self, struct t_tune *tune);
static int	t_action_set(struct t_action *self, struct t_tune *tune);
static int	t_action_save(struct t_action *self, struct t_tune *tune);


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
			arg++; /* skip : */
		} else /* if t->argc > 1 */
			ABANDON_SHIP();

		/* FIXME: maybe a flag to know if we need to save before the
		   action ? */
		switch (t->kind) {
		case T_ACTION_RENAME:
			a = t_action_new(T_ACTION_SAVE, NULL);
			if (a == NULL)
				goto error_label;
			TAILQ_INSERT_TAIL(aQ, a, entries);
			/* FALLTHROUGH */
		case T_ACTION_ADD: /* FALLTHROUGH */
		case T_ACTION_CLEAR: /* FALLTHROUGH */
		case T_ACTION_EDIT: /* FALLTHROUGH */
		case T_ACTION_LOAD: /* FALLTHROUGH */
		case T_ACTION_SET: /* FALLTHROUGH */
		case T_ACTION_PRINT: /* FALLTHROUGH */
		case T_ACTION_BACKEND:
			a = t_action_new(t->kind, arg);
			if (a == NULL)
				goto error_label;
			TAILQ_INSERT_TAIL(aQ, a, entries);
			break;
		default: /* private action */
			ABANDON_SHIP();
		}
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

	/* check if write access is needed */
	write = 0;
	TAILQ_FOREACH(a, aQ, entries)
		write += a->write;
	if (write > 0) {
		a = t_action_new(T_ACTION_SAVE, NULL);
		if (a == NULL)
			goto error_label;
		TAILQ_INSERT_TAIL(aQ, a, entries);
	}

	if (write_p != NULL)
		*write_p = write;
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
		/* FIXME: error checking on t_rename_parse() */
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
	case T_ACTION_SAVE:
		/* we don't set write() because T_ACTION_SAVE is private */
		a->apply = t_action_save;
		break;
	default: ABANDON_SHIP();
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
	int i;
	struct token **tknv;

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
		tknv = victim->opaque;
		for (i = 0; tknv[i]; i++)
			free(tknv[i]);
		free(tknv);
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
	assert_not_null(tune);
	assert(self->kind == T_ACTION_ADD);

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

	(void)printf("%s %s\n", tune->backend->libid, tune->path);
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
	assert_not_null(tune);
	assert(self->kind == T_ACTION_CLEAR);

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
	FILE *fp = NULL;
	struct t_taglist *tlist = NULL;
	char *tmp = NULL, *yaml = NULL;
	const char *editor;
	pid_t editpid; /* child process */
	int status;
	struct stat before, after;

	assert_not_null(self);
	assert_not_null(tune);
	assert(self->kind == T_ACTION_EDIT);

	/* convert the tags into YAML */
	tlist = t_tune_tags(tune);
	if (tlist == NULL)
		goto error_label;
	yaml = t_tags2yaml(tlist, tune->path);
	t_taglist_delete(tlist);
	tlist = NULL;
	if (yaml == NULL)
		goto error_label;

	/* print the YAML into a temp file */
	if (asprintf(&tmp, "/tmp/%s-XXXXXX.yml", getprogname()) < 0)
		goto error_label;
	if (mkstemps(tmp, 4) == -1) {
		warn("mkstemps");
		goto error_label;
	}
	fp = fopen(tmp, "w");
	if (fp == NULL) {
		warn("%s: fopen", tmp);
		goto error_label;
	}
	if (fprintf(fp, "%s", yaml) < 0) {
		warn("%s: fprintf", tmp);
		goto error_label;
	}
	if (fclose(fp) != 0) {
		warn("%s: fclose", tmp);
		goto error_label;
	}
	fp = NULL;

	/* call the user's editor to edit the temp file */
	editor = getenv("EDITOR");
	if (editor == NULL) {
		warnx("please set the $EDITOR environment variable.");
		goto error_label;
	}
	if (stat(tmp, &before) != 0)
		goto error_label;
	switch (editpid = fork()) {
	case -1:
		warn("fork");
		goto error_label;
		/* NOTREACHED */
	case 0: /* child (edit process) */
		/*
		 * we're actually so cool, that we keep the user waiting if
		 * $EDITOR start slowly. The slow-editor-detection-algorithm
		 * used might not be the best known at the time of writing, but
		 * it has shown really good results and is pretty clear.
		 */
		if (strcmp(editor, "emacs") == 0)
			(void)fprintf(stderr, "Starting %s, please wait...\n", editor);
		execlp(editor, /* argv[0] */editor, /* argv[1] */tmp, NULL);
		err(errno, "execlp");
		/* NOTREACHED */
	default: /* parent (tagutil process) */
		waitpid(editpid, &status, 0);
	}
	if (stat(tmp, &after) != 0)
		goto error_label;

	/* check if the edit process went successfully */
	if (after.st_mtime > before.st_mtime &&
	    WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		struct t_action *load = t_action_new(T_ACTION_LOAD, tmp);
		if (load == NULL)
			goto error_label;
		status = load->apply(load, tune);
		t_action_delete(load);
		if (status != 0)
			goto error_label;
	}

	(void)unlink(tmp);
	free(tmp);
	free(yaml);
	return (0);
error_label:
	if (fp != NULL)
		(void)fclose(fp);
	if (tmp != NULL)
		(void)unlink(tmp);
	free(tmp);
	free(yaml);
	return (-1);
}


static int
t_action_load(struct t_action *self, struct t_tune *tune)
{
	FILE *fp;
	const char *yamlfile;
	char *errmsg;
	struct t_taglist *tlist;
	int r;

	assert_not_null(self);
	assert_not_null(tune);
	assert(self->kind == T_ACTION_LOAD);

	yamlfile = self->opaque;
	if (strcmp(yamlfile, "-") == 0)
		fp = stdin;
	else {
		fp = fopen(yamlfile, "r");
		if (fp == NULL) {
			warn("%s: fopen", yamlfile);
			return (-1);
		}
	}

	tlist = t_yaml2tags(fp, &errmsg);
	if (fp != stdin)
		(void)fclose(fp);
	if (tlist == NULL) {
		warnx("%s", errmsg);
		free(errmsg);
		return (-1);
	 }
	r = t_tune_set_tags(tune, tlist);
	t_taglist_delete(tlist);
	return (r);
}


static int
t_action_print(struct t_action *self, struct t_tune *tune)
{
	char	*yaml;
	struct t_taglist *tlist;

	assert_not_null(self);
	assert_not_null(tune);
	assert(self->kind == T_ACTION_PRINT);

	tlist = t_tune_tags(tune);
	if (tlist == NULL)
		return (-1);
	yaml = t_tags2yaml(tlist, tune->path);
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
	int ret = 0;
	const char *ext;
	char *npath = NULL, *rname = NULL, *q = NULL;
	const char *opath;
	const char *dirn;
	struct t_token **tknv;

	assert_not_null(self);
	assert_not_null(tune);
	assert(self->kind == T_ACTION_RENAME);
	assert(!tune->dirty);

	tknv = self->opaque;

	ext = strrchr(tune->path, '.');
	if (ext == NULL) {
		warnx("%s: can not find file extension", tune->path);
		goto error_label;
	}
	ext++; /* skip dot */
	/* FIXME: t_rename_eval() error handling ? */
	rname = t_rename_eval(tune, tknv);
	if (rname == NULL)
		goto error_label;

	/* rname is now OK. store into result the full new path.  */
	dirn = t_dirname(tune->path);
	if (dirn == NULL) {
		warn("dirname");
		goto error_label;
	}

	opath = tune->path;
	/* we dont want foo.flac to be renamed then same name just with a
	   different path like ./foo.flac */
	if (strcmp(opath, t_basename(opath)) == 0) {
		if (asprintf(&npath, "%s.%s", rname, ext) < 0)
			goto error_label;
	} else {
		if (asprintf(&npath, "%s/%s.%s", dirn, rname, ext) < 0)
			goto error_label;
	}

	/* ask user for confirmation and rename if user want to */
	if (asprintf(&q, "rename `%s' to `%s'", opath, npath) < 0)
		goto error_label;
	if (strcmp(opath, npath) != 0 && t_yesno(q)) {
		ret = t_rename_safe(opath, npath);
		if (ret == 0) {
			/* do a full reload of the file */
			struct t_tune tmp;
			struct t_tune *neo = t_tune_new(npath);
			if (neo == NULL)
				goto error_label;

			/* switch */
			memcpy(&tmp, tune, sizeof(struct t_tune));
			memcpy(tune, neo,  sizeof(struct t_tune));
			memcpy(neo,  &tmp, sizeof(struct t_tune));
			/* now neo is in fact the "old" one */
			t_tune_delete(neo);
		}
	}

	free(q);
	free(npath);
	free(rname);
	return (ret);
error_label:
	free(q);
	free(npath);
	free(rname);
	return (-1);
}


static int
t_action_set(struct t_action *self, struct t_tune *tune)
{
	struct t_tag *t, *t_tmp, *neo;
	struct t_taglist *tlist;
	int n, status;

	assert_not_null(self);
	assert_not_null(tune);
	assert(self->kind == T_ACTION_SET);

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


static int
t_action_save(struct t_action *self, struct t_tune *tune)
{

	assert_not_null(self);
	assert_not_null(tune);
	assert(self->kind == T_ACTION_SAVE);

	return (t_tune_save(tune) == 0 ? 0 : -1);
}


static int
t_yesno(const char *question)
{
	extern int	Yflag, Nflag;
	char		*endl;
	char		buffer[5]; /* strlen("yes\n\0") == 5 */

	for (;;) {
		if (feof(stdin) && !Yflag && !Nflag)
			return (0);

		(void)memset(buffer, '\0', sizeof(buffer));

		if (question != NULL) {
			(void)printf("%s? [y/n] ", question);
			(void)fflush(stdout);
		}

		if (Yflag) {
			(void)printf("y\n");
			return (1);
		} else if (Nflag) {
			(void)printf("n\n");
			return (0);
		}

		if (fgets(buffer, NELEM(buffer), stdin) == NULL) {
			if (feof(stdin))
				return (0);
			else
				err(errno, "fgets");
		}

		endl = strchr(buffer, '\n');
		if (endl == NULL) {
			/* buffer didn't receive EOL, must still be on stdin */
			while (getc(stdin) != '\n' && !feof(stdin))
				continue;
		} else {
			*endl = '\0';
			(void)t_strtolower(buffer);
			if (strcmp(buffer, "n") == 0 || strcmp(buffer, "no") == 0)
				return (0);
			else if (strcmp(buffer, "y") == 0 || strcmp(buffer, "yes") == 0)
				return (1);
		}
	}
}

/*
 * t_editor.c
 *
 * editor routines for tagutil.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "t_config.h"
#include "t_editor.h"

#include "t_yaml.h"
#include "t_loader.h"


int
t_edit(struct t_tune *tune)
{
	FILE *fp = NULL;
	struct t_taglist *tlist = NULL;
	char *tmp = NULL, *yaml = NULL;
	const char *editor, *tmpdir;
	pid_t editpid; /* child process */
	int status;
	struct stat before, after;

	assert_not_null(tune);

	/* convert the tags into YAML */
	tlist = t_tune_tags(tune);
	if (tlist == NULL)
		goto error_label;
	yaml = t_tags2yaml(tlist, t_tune_path(tune));
	t_taglist_delete(tlist);
	tlist = NULL;
	if (yaml == NULL)
		goto error_label;

	tmpdir = getenv("TMPDIR");
	if (tmpdir == NULL)
		tmpdir = "/tmp";
	/* print the YAML into a temp file */
	if (asprintf(&tmp, "%s/%s-XXXXXX.yml", tmpdir, getprogname()) < 0)
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
		if (t_load(tune, tmp) == -1)
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

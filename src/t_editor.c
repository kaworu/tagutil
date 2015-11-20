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
#include "t_format.h"
#include "t_editor.h"
#include "t_loader.h"


int
t_edit(struct t_tune *tune)
{
	FILE *fp = NULL;
	struct t_taglist *tlist = NULL;
	char *tmp = NULL, *fmtdata = NULL;
	const char *editor, *tmpdir;
	pid_t editpid; /* child process */
	int status;
	struct stat before, after;
	extern const struct t_format *Fflag;

	assert(tune != NULL);

	/* convert the tags into the requested format */
	tlist = t_tune_tags(tune);
	if (tlist == NULL)
		goto error_label;
	fmtdata = Fflag->tags2fmt(tlist, t_tune_path(tune));
	t_taglist_delete(tlist);
	tlist = NULL;
	if (fmtdata == NULL)
		goto error_label;

	tmpdir = getenv("TMPDIR");
	if (tmpdir == NULL)
		tmpdir = "/tmp";
	/* print the format data into a temp file */
	if (asprintf(&tmp, "%s/%s-XXXXXX.%s", tmpdir, getprogname(),
	    Fflag->fileext) < 0) {
		goto error_label;
	}
	if (mkstemps(tmp, strlen(Fflag->fileext) + 1) == -1) {
		warn("mkstemps");
		goto error_label;
	}
	fp = fopen(tmp, "w");
	if (fp == NULL) {
		warn("%s: fopen", tmp);
		goto error_label;
	}
	if (fprintf(fp, "%s", fmtdata) < 0) {
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

	/* save the current mtime so we know later if the file has been
	   modified */
	if (stat(tmp, &before) != 0)
		goto error_label;

	/* launch the editor */
	switch (editpid = fork()) {
	case -1: /* error */
		warn("fork");
		goto error_label;
		/* NOTREACHED */
	case 0: /* child (edit process) */
		execlp(editor, /* argv[0] */editor, /* argv[1] */tmp, NULL);
		/* if we reach here, execlp(3) has failed */
		err(EXIT_FAILURE, "execlp");
		/* NOTREACHED */
	default: /* parent (tagutil process) */
		waitpid(editpid, &status, 0);
	}

	/* get the mtime now that the editor has been run */
	if (stat(tmp, &after) != 0)
		goto error_label;

	/* we perform the load iff the file has been modified by the edit
	   process and that process exited with success */
	if (after.st_mtime > before.st_mtime &&
	    WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		if (t_load(tune, tmp) == -1)
			goto error_label;
	}

	(void)unlink(tmp);
	free(tmp);
	free(fmtdata);
	return (0);
error_label:
	if (fp != NULL)
		(void)fclose(fp);
	if (tmp != NULL)
		(void)unlink(tmp);
	free(tmp);
	free(fmtdata);
	return (-1);
}

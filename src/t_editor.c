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
	int status, success = 0;
	struct stat before, after;
	extern const struct t_format *Fflag;

	assert(tune != NULL);

	/* convert the tags into the requested format */
	tlist = t_tune_tags(tune);
	if (tlist == NULL)
		goto out;
	fmtdata = Fflag->tags2fmt(tlist, t_tune_path(tune));
	t_taglist_delete(tlist);
	tlist = NULL;
	if (fmtdata == NULL)
		goto out;

	tmpdir = getenv("TMPDIR");
	if (tmpdir == NULL)
		tmpdir = "/tmp";
	/* print the format data into a temp file */
	if (asprintf(&tmp, "%s/%s-XXXXXX.%s", tmpdir, getprogname(),
	    Fflag->fileext) < 0) {
		goto out;
	}
	if (mkstemps(tmp, strlen(Fflag->fileext) + 1) == -1) {
		warn("mkstemps");
		goto out;
	}
	fp = fopen(tmp, "w");
	if (fp == NULL) {
		warn("%s: fopen", tmp);
		goto out;
	}
	if (fprintf(fp, "%s", fmtdata) < 0) {
		warn("%s: fprintf", tmp);
		goto out;
	}
	if (fclose(fp) != 0) {
		warn("%s: fclose", tmp);
		goto out;
	}
	fp = NULL;

	/* call the user's editor to edit the temp file */
	editor = getenv("EDITOR");
	if (editor == NULL) {
		warnx("please set the $EDITOR environment variable.");
		goto out;
	}

	/* save the current mtime so we know later if the file has been
	   modified */
	if (stat(tmp, &before) != 0)
		goto out;

	/* launch the editor */
	switch (editpid = fork()) {
	case -1: /* error */
		warn("fork");
		goto out;
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
		goto out;

	int modified = (after.st_mtim.tv_sec  > before.st_mtim.tv_sec ||
		        after.st_mtim.tv_nsec > before.st_mtim.tv_nsec);

	/* we perform the load iff the file has been modified by the edit
	   process and that process exited with success */
	if (modified && WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		if (t_load(tune, tmp) == -1)
			goto out;
	}

	success = 1;
	/* FALLTHROUGH */
out:
	if (fp != NULL)
		(void)fclose(fp);
	if (tmp != NULL)
		(void)unlink(tmp);
	free(tmp);
	free(fmtdata);
	return (success ? 0 : -1);
}

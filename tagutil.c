/*
 *  _                    _   _ _
 * | |_ __ _  __ _ _   _| |_(_) |
 * | __/ _` |/ _` | | | | __| | |
 * | || (_| | (_| | |_| | |_| | |
 *  \__\__,_|\__, |\__,_|\__|_|_|
 *           |___/
 *
 * FIXME:
 * tagutil is a simple command line tool to edit music file's tag. It use
 * taglib (http://developer.kde.org/~wheeler/taglib.html) to get and set music
 * file's tags so be sure to install it before trying to compile tagutil.
 * for a help lauch tagutil without argument.
 *
 * Copyright (c) 2008-2009, Alexandre Perrin <kaworu@kaworu.ch>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <unistd.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_tag.h"
#include "t_file.h"
#include "t_backend.h"
#include "t_action.h"
#include "t_yaml.h"
#include "tagutil.h"


/*
 * get action with getopt(3) and then apply it to all files given in argument.
 * usage() is called if an error is detected.
 */
int
main(int argc, char *argv[])
{
    	int	i, retval;
	bool	rw; /* read and write access needed */
    	char	*path;
    	struct t_file		*file;
	const struct t_backend	*b;
	const struct t_backendQ	*bQ;
	struct t_action		*a;
	struct t_actionQ	*aQ;

	errno = 0;
	bQ = t_get_backend();
	aQ = t_actionQ_create(&argc, &argv);

	if (argc == 0) {
		errx(EINVAL, "missing file argument.\nrun `%s -h' to see help.",
		    getprogname());
    	}
	if (TAILQ_EMPTY(aQ))
		/* no action given, fallback to default */
		TAILQ_INSERT_TAIL(aQ, t_action_new(T_SHOW, NULL), next);

	/* check if write access is needed */
	rw = false;
	TAILQ_FOREACH(a, aQ, next) {
		if (rw = a->rw)
			break;
	}
	if (rw)
		TAILQ_INSERT_TAIL(aQ, t_action_new(T_SAVE_IF_DIRTY, NULL), next);

	/*
	 * main loop, foreach files
	 */
	retval = EXIT_SUCCESS;
	for (i = 0; i < argc; i++) {
		/* check file path and access */
		path = argv[i];
		if (access(path, (rw ? (R_OK | W_OK) : R_OK)) == -1) {
			warn("%s", path);
			retval = EINVAL;
			continue;
		}

		/* try every backend in the right order */
		file = NULL;
		TAILQ_FOREACH(b, bQ, next) {
			file = (*b->ctor)(path);
			if (file != NULL)
				break;
		}
		if (file == NULL) {
			warnx("`%s' unsupported file format", path);
			retval = EINVAL;
			continue;
		}

		/* apply every action asked to the file */
		TAILQ_FOREACH(a, aQ, next) {
			bool ok = (*a->apply)(a, &file);
			if (!ok) {
				warnx("`%s': %s", file->path, t_error_msg(file));
				break;
			}
		}
        	file->destroy(file);
	}
	t_actionQ_destroy(aQ);
	return (retval);
}


/* FIXME: error handling with stat(2) ? */
bool
user_edit(const char *restrict path)
{
	pid_t	edit; /* child process */
	int	status;
	time_t	before;
	time_t	after;
	struct stat s;
	char	*editor;

	assert_not_null(path);

	editor = getenv("EDITOR");
	if (editor == NULL)
		errx(-1, "please, set the $EDITOR environment variable.");
	/*
	 * we're actually so cool, that we keep the user waiting if $EDITOR
	 * start slowly. The slow-editor-detection-algorithm used maybe not
	 * the best known at the time of writing, but it has shown really good
	 * results and is pretty short and clear.
	 */
	if (strcmp(editor, "emacs") == 0)
		(void)fprintf(stderr, "Starting %s, please wait...\n", editor);

        if (stat(path, &s) != 0)
		return (false);
	before = s.st_mtime;
	switch (edit = fork()) {
	case -1:
		err(errno, "fork");
		/* NOTREACHED */
	case 0:
		/* child (edit process) */
		execlp(editor, /* argv[0] */editor, /* argv[1] */path, NULL);
		err(errno, "execlp");
		/* NOTREACHED */
	default:
		/* parent (tagutil process) */
		waitpid(edit, &status, 0);
	}

        if (stat(path, &s) != 0)
		return (false);
	after = s.st_mtime;
	if (before == after)
		/* the file hasn't been modified */
		return (false);

	return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
}


bool
tagutil_print(struct t_file *restrict file)
{
    	char	*yaml;

    	assert_not_null(file);

	yaml = t_tags2yaml(file);
	if (yaml == NULL)
		return (false);
	(void)printf("%s\n", yaml);
	freex(yaml);
	return (true);
}


bool
tagutil_load(struct t_file *restrict file, const char *restrict path)
{
	bool	retval = true;
	FILE	*stream;
	struct t_taglist *T;

	assert_not_null(file);
	assert_not_null(path);

	if (strcmp(path, "-") == 0)
		stream = stdin;
	else
		stream = xfopen(path, "r");

	T = t_yaml2tags(file, stream);
	if (stream != stdin)
		xfclose(stream);
	if (T == NULL)
		return (false);
	retval = file->clear(file, NULL) && file->add(file, T) && file->save(file);
	t_taglist_destroy(T);
	return (retval);
}


bool
tagutil_edit(struct t_file *restrict file)
{
	bool	retval = true;
	char	*tmp_file, *yaml, *editor;
	FILE	*stream;

	assert_not_null(file);

	yaml = t_tags2yaml(file);
	if (yaml == NULL)
		return (false);

	(void)printf("%s\n", yaml);

	if (t_yesno("edit this file")) {
		tmp_file = t_mkstemp("/tmp");
		stream = xfopen(tmp_file, "w");
		(void)fprintf(stream, "%s", yaml);
		editor = getenv("EDITOR");
		if (editor != NULL && strcmp(editor, "vim") == 0)
			(void)fprintf(stream, "\n# vim:filetype=yaml");
		xfclose(stream);
		if (user_edit(tmp_file))
			retval = tagutil_load(file, tmp_file);
		xunlink(tmp_file);
		freex(tmp_file);
	}
	freex(yaml);
	return (retval);
}


bool
tagutil_rename(struct t_file *restrict file, struct t_token **restrict tknary)
{
	bool	retval;
    	char	*ext, *result, *dirn, *fname, *question;

    assert_not_null(file);
    assert_not_null(tknary);
    t_error_clear(file);

    ext = strrchr(file->path, '.');
    if (ext == NULL) {
        t_error_set(file, "can't find file extension for `%s'", file->path);
        return (false);
    }
    ext++; /* skip dot */
    fname = t_rename_eval(file, tknary);
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
        (void)xasprintf(&question, "rename `%s' to `%s'", file->path, result);
        if (t_yesno(question))
		retval = t_rename_safe(file, result);
        freex(question);
    }
    freex(result);
    return (retval);
}


bool
tagutil_filter(struct t_file *restrict file,
        const struct t_ast *restrict ast)
{
    bool retval;

    assert_not_null(file);
    assert_not_null(ast);

    retval = t_filter_eval(file, ast);

    if (retval)
        (void)printf("%s\n", file->path);

    return (retval);
}


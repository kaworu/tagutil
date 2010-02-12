/*
 * t_toolkit.c
 *
 * handy functions toolkit for tagutil.
 *
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_action.h"



bool
t_yesno(const char *restrict question)
{
    char *endl;
    char buffer[5]; /* strlen("yes\n\0") == 5 */

    for (;;) {
        if (feof(stdin) && !Yflag && !Nflag)
            return (false);

        (void)memset(buffer, '\0', sizeof(buffer));

        if (question != NULL) {
            (void)printf("%s? [y/n] ", question);
            (void)fflush(stdout);
        }

        if (Yflag) {
            (void)printf("y\n");
            return (true);
        }
        else if (Nflag) {
            (void)printf("n\n");
            return (false);
        }

        if (fgets(buffer, countof(buffer), stdin) == NULL) {
            if (feof(stdin))
                return (false);
            else
                err(errno, "fgets");
        }

        endl = strchr(buffer, '\n');
        if (endl == NULL) {
        /* buffer didn't receive EOL, must still be on stdin */
            while (getc(stdin) != '\n' && !feof(stdin))
                continue;
        }
        else {
            *endl = '\0';
            (void)t_strtolower(buffer);
            if (strcmp(buffer, "n") == 0 || strcmp(buffer, "no") == 0)
                return (false);
            else if (strcmp(buffer, "y") == 0 || strcmp(buffer, "yes") == 0)
                return (true);
        }
    }
}


bool
t_user_edit(const char *restrict path)
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


/* $OpenBSD: dirname.c,v 1.13 2005/08/08 08:05:33 espie Exp $
 *
 * Copyright (c) 1997, 2004 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <sys/param.h>

/*
 * OpenBSD's dirname modified to return a malloc()d pointer (or NULL).
 */
char *
t_dirname(const char *path)
{
	size_t len;
	const char *endp;
    char *dname;

    dname = xmalloc(MAXPATHLEN);

	/* Empty or NULL string gets treated as "." */
	if (path == NULL || *path == '\0') {
		dname[0] = '.';
		dname[1] = '\0';
		return (dname);
	}

	/* Strip any trailing slashes */
	endp = path + strlen(path) - 1;
	while (endp > path && *endp == '/')
		endp--;

	/* Find the start of the dir */
	while (endp > path && *endp != '/')
		endp--;

	/* Either the dir is "/" or there are no slashes */
	if (endp == path) {
		dname[0] = *endp == '/' ? '/' : '.';
		dname[1] = '\0';
		return (dname);
	} else {
		/* Move forward past the separating slashes */
		do {
			endp--;
		} while (endp > path && *endp == '/');
	}

	len = endp - path + 1;
	if (len >= MAXPATHLEN) {
		errno = ENAMETOOLONG;
        	free(dname);
		return (NULL);
	}
	memcpy(dname, path, len);
	dname[len] = '\0';
	return (dname);
}

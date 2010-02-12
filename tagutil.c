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
#include "t_config.h"
#include "t_toolkit.h"
#include "t_file.h"
#include "t_backend.h"
#include "t_action.h"


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
	if (TAILQ_EMPTY(aQ)) {
	/* no action given, fallback to default */
		a = t_action_new(T_SHOW, NULL);
		TAILQ_INSERT_TAIL(aQ, a, entries);
	}

	/* check if write access is needed */
	rw = false;
	TAILQ_FOREACH(a, aQ, entries) {
		if ((rw = a->rw)) {
			a = t_action_new(T_SAVE_IF_DIRTY, NULL);
			TAILQ_INSERT_TAIL(aQ, a, entries);
		}
	}

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
		TAILQ_FOREACH(b, bQ, entries) {
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
		TAILQ_FOREACH(a, aQ, entries) {
			bool ok = (*a->apply)(a, file);
			if (!ok)
				/* FIXME: warnx("`%s': %s", file->path, t_error_msg(file));*/
				break;
		}
        	file->destroy(file);
	}
	t_actionQ_destroy(aQ);
	return (retval);
}

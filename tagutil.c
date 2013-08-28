/*  _                    _   _ _
 * | |_ __ _  __ _ _   _| |_(_) |
 * | __/ _` |/ _` | | | | __| | |
 * | || (_| | (_| | |_| | |_| | |
 *  \__\__,_|\__, |\__,_|\__|_|_|
 *           |___/
 *
 * tagutil is a simple command line tool to edit music file's tag. It use
 * TagLib, libvorbis and / or libflac  to get and set music file's tags.
 * for help, launch tagutil without argument and enjoy the show.
 *
 * The license is a BSD 2-Clause, see COPYING.
 */
#include "t_config.h"
#include "t_toolkit.h"
#include "t_file.h"
#include "t_backend.h"
#include "t_action.h"


/* options */
bool	dflag = false; /* create directory with rename */
bool	Nflag = false; /* answer no to all questions */
bool	Yflag = false; /* answer yes to all questions */


/*
 * get action with getopt(3) and then apply it to all files given in argument.
 * usage() is called if an error is detected.
 */
int
main(int argc, char *argv[])
{
    	int	i, retval;
	bool	write; /* write access needed */
    	struct t_file		*file;
	const struct t_backend	*b;
	const struct t_backendQ	*bQ;
	struct t_action		*a;
	struct t_actionQ	*aQ;

	errno = 0;

	while ((i = getopt(argc, argv, "dhNY")) != -1) {
		switch ((char)i) {
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

	aQ = t_actionQ_create(&argc, &argv, &write);

	if (argc == 0) {
		errx(EINVAL, "missing file argument.\nrun `%s -h' to see help.",
		    getprogname());
    	}

	/*
	 * main loop, foreach files
	 */
	bQ = t_all_backends();
	retval = EXIT_SUCCESS;
	for (i = 0; i < argc; i++) {
		/* check file path and access */
		const char *path = argv[i];
		if (access(path, (write ? (R_OK | W_OK) : R_OK)) == -1) {
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
				warnx("%s: %s", file->path, t_error_msg(file));
				break;
		}
        	file->destroy(file);
	}
	t_actionQ_destroy(aQ);
	return (retval);
}

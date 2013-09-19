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
#include "t_tune.h"
#include "t_backend.h"
#include "t_action.h"


/*
 * show usage and exit.
 */
void usage(void) t__dead2;


/* options */
int	dflag; /* create directory with rename */
int	Nflag; /* answer no to all questions */
int	Yflag; /* answer yes to all questions */


/*
 * get action with getopt(3) and then apply it to all files given in argument.
 * usage() is called if an error is detected.
 */
int
main(int argc, char *argv[])
{
    	int	i, retval;
	int	write; /* write access needed */
    	struct t_tune		 tune;
	struct t_action		*a;
	struct t_actionQ	*aQ;

	errno = 0; /* this is a bug in malloc(3) */

	while ((i = getopt(argc, argv, "dhNY")) != -1) {
		switch ((char)i) {
		case 'd':
			dflag = 1;
			break;
		case 'N':
			if (Yflag)
				err(EINVAL, "cannot set both -Y and -N");
			Nflag = 1;
			break;
		case 'Y':
			if (Nflag)
				err(EINVAL, "cannot set both -Y and -N");
			Yflag = 1;
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

	aQ = t_actionQ_new(&argc, &argv, &write);
	if (aQ == NULL) {
		if (errno == ENOMEM)
			err(ENOMEM, "malloc");
		else /* if EINVAL */ {
			(void)fprintf(stderr, "Try `%s -h' for help.\n",
			    getprogname());
			exit(EINVAL);
		}
		/* NOTREACHED */
	}

	if (argc == 0) {
		errx(EINVAL, "missing file argument.\nTry `%s -h' for help.",
		    getprogname());
	}

	/*
	 * main loop, foreach files
	 */
	retval = EXIT_SUCCESS;
	for (i = 0; i < argc; i++) {
		/* check file path and access */
		const char *path = argv[i];
		if (access(path, (write ? (R_OK | W_OK) : R_OK)) == -1) {
			warn("%s", path);
			retval = EINVAL;
			continue;
		}

		if (t_tune_init(&tune, path) != 0) {
			if (errno == ENOMEM)
				err(ENOMEM, "malloc");
			warnx("%s: unsupported file format", path);
			retval = EINVAL;
			continue;
		}

		/* apply every actions */
		TAILQ_FOREACH(a, aQ, entries) {
			if (a->apply(a, &tune) != 0) {
				/*
				 * prevent further action on this particular
				 * file.
				 */
				break;
			}
		}
		t_tune_clear(&tune);
	}
	t_actionQ_delete(aQ);
	return (retval);
}


/*
 * show usage and exit.
 */
void
usage(void)
{
	const struct t_backendQ *bQ = t_all_backends();
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

	exit(1);
}

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
 * tagutil is under a BSD 2-Clause license, see LICENSE.
 */
#include "t_config.h"
#include "t_toolkit.h"
#include "t_tune.h"
#include "t_backend.h"
#include "t_format.h"
#include "t_action.h"


/*
 * show usage and exit.
 */
void usage(void) t__dead2;


/* options */
int			 dflag; /* create directory with rename */
const struct t_format	*Fflag; /* output format */
int			 Nflag; /* answer no to all questions */
int			 Yflag; /* answer yes to all questions */


/*
 * get action with getopt(3) and then apply it to all files given in argument.
 * usage() is called if an error is detected.
 */
int
main(int argc, char *argv[])
{
	int	i, retval;
	int	write = 0; /* write access needed ? */
	struct t_tune		*tune;
	struct t_action		*a;
	struct t_format		*fmt;
	struct t_actionQ	*aQ;

	errno = 0; /* this is a bug in malloc(3) */

	/* we assert because only the build system ensure that tagutil is build
	   against at least libyaml as format. */
	assert(Fflag = TAILQ_FIRST(t_all_formats()));

	while ((i = getopt(argc, argv, "dhF:NY")) != -1) {
		switch ((char)i) {
		case 'd':
			dflag = 1;
			break;
		case 'F':
			Fflag = NULL;
			TAILQ_FOREACH(fmt, t_all_formats(), entries) {
				if (strcmp(fmt->fileext, optarg) == 0) {
					Fflag = fmt;
					break;
				}
			}
			if (Fflag == NULL) {
				errx(errno = EINVAL, "%s: invalid -F option, "
				    "try %s -h' to see supported formats.",
				    optarg, getprogname());
			}
			break;
		case 'N':
			if (Yflag)
				err(errno = EINVAL, "cannot set both -Y and -N");
			Nflag = 1;
			break;
		case 'Y':
			if (Nflag)
				err(errno = EINVAL, "cannot set both -Y and -N");
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

	aQ = t_actionQ_new(&argc, &argv);
	if (aQ == NULL) {
		if (errno == ENOMEM)
			err(ENOMEM, "malloc");
		else /* if EINVAL */ {
			fprintf(stderr, "Try `%s -h' for help.\n",
			    getprogname());
			exit(EINVAL);
		}
		/* NOTREACHED */
	}

	if (argc == 0) {
		errx(EINVAL, "missing file argument.\nTry `%s -h' for help.",
		    getprogname());
	}

	/* find if any action need write access */
	TAILQ_FOREACH(a, aQ, entries)
		write += a->write;

	/*
	 * main loop, foreach files
	 */
	retval = EXIT_SUCCESS;
	for (i = 0; i < argc; i++) {
		/* check file path and access */
		const char *path = argv[i];
		int success = 1;
		if (access(path, (write ? (R_OK | W_OK) : R_OK)) == -1) {
			warn("%s", path);
			retval = EINVAL;
			continue;
		}

		if ((tune = t_tune_new(path)) == NULL) {
			if (errno == ENOMEM)
				err(ENOMEM, "malloc");
			warnx("%s: unsupported file format", path);
			retval = EINVAL;
			continue;
		}

		/* apply every actions */
		TAILQ_FOREACH(a, aQ, entries) {
			if (a->apply(a, tune) != 0) {
				/*
				 * prevent further action on this particular
				 * file.
				 */
				success = 0;
				break;
			}
		}
		if (write && success) {
			/* all actions went well and at least one of them
			   require the tags to be written back to the file */
			if (t_tune_save(tune) == -1) {
				warnx("%s: could not write tags to the file,",
				    t_tune_path(tune));
			}
		}
		t_tune_delete(tune);
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
	const struct t_formatQ	*fmtQ = t_all_formats();
	const struct t_backendQ	*bQ   = t_all_backends();
	const struct t_format	*fmt;
	const struct t_backend	*b;

	fprintf(stderr, "tagutil v"T_TAGUTIL_VERSION "\n\n");
	fprintf(stderr, "usage: %s [OPTION]... [ACTION:ARG]... [FILE]...\n",
	    getprogname());
	fprintf(stderr, "Modify or display music file's tag.\n");
	fprintf(stderr, "\n");

	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -h     show this help\n");
	fprintf(stderr, "  -p     create destination directories if needed (used by rename)\n");
	fprintf(stderr, "  -F fmt use the fmt format for print, edit and load actions (see Formats)\n");
	fprintf(stderr, "  -Y     answer yes to all questions\n");
	fprintf(stderr, "  -N     answer no  to all questions\n");
	fprintf(stderr, "\n");

	fprintf(stderr, "Actions:\n");
	fprintf(stderr, "  print            print tags (default action)\n");
	fprintf(stderr, "  backend          print the backend used (see Backend)\n");
	fprintf(stderr, "  clear:TAG        clear all tag TAG. If TAG is "
	    "empty, all tags are cleared\n");
	fprintf(stderr, "  add:TAG=VALUE    add a TAG=VALUE pair\n");
	fprintf(stderr, "  set:TAG=VALUE    set TAG to VALUE\n");
	fprintf(stderr, "  edit             prompt for editing\n");
	fprintf(stderr, "  load:PATH        load PATH yaml tag file\n");
	fprintf(stderr, "  rename:PATTERN   rename to PATTERN\n");
	fprintf(stderr, "\n");

	fprintf(stderr, "Formats:\n");
	TAILQ_FOREACH(fmt, fmtQ, entries)
		fprintf(stderr, "  %10s: %s\n", fmt->fileext, fmt->desc);
	fprintf(stderr, "\n");

	fprintf(stderr, "Backends:\n");
	TAILQ_FOREACH(b, bQ, entries)
		fprintf(stderr, "  %10s: %s\n", b->libid, b->desc);
	fprintf(stderr, "\n");

	exit(1);
}

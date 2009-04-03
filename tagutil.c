/*
 *  _                    _   _ _
 * | |_ __ _  __ _ _   _| |_(_) |
 * | __/ _` |/ _` | | | | __| | |
 * | || (_| | (_| | |_| | |_| | |
 *  \__\__,_|\__, |\__,_|\__|_|_|
 *           |___/
 *
 * tagutil is a simple command line tool to edit music file's tag. It use
 * taglib (http://developer.kde.org/~wheeler/taglib.html) to get and set music
 * file's tags so be sure to install it before trying to compile tagutil.
 * for a help lauch tagutil without argument.
 *
 * Copyright (c) 2008, Perrin Alexandre <kaworu@kaworu.ch>
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "t_config.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <unistd.h> /* getopt(3) */

#include <tag_c.h>

#include "t_lexer.h"
#include "t_parser.h"
#include "t_interpreter.h"
#include "t_yaml.h"
#include "t_renamer.h"
#include "t_toolkit.h"

#include "tagutil.h"


bool        pflag = false; /* display tags action */
bool        Yflag = false; /* yes answer to all questions */
bool        Nflag = false; /* no  answer to all questions */
bool        eflag = false; /* edit */
bool        dflag = false; /* create directory with rename */
bool        rflag = false;  /* rename */
bool        xflag = false;  /* filter */
bool        aflag = false;  /* set album */
bool        Aflag = false;  /* set artist */
bool        cflag = false;  /* set comment */
bool        gflag = false;  /* set genre */
bool        Tflag = false;  /* set track */
bool        tflag = false;  /* set title */
bool        yflag = false;  /* set year */

char       *r_arg; /* rename pattern */
struct ast *x_arg; /* filter code */
char       *a_arg; /* album argument */
char       *A_arg; /* artist argument */
char       *c_arg; /* comment argument */
char       *g_arg; /* genre argument */
int         T_arg;    /* track argument */
char       *t_arg; /* title argument */
int         y_arg;    /* year argument */
/*
 * get action with getopt(3) and then apply it to all files given in argument.
 * usage() is called if an error is detected.
 */
int
main(int argc, char *argv[])
{
    bool w = false;
    int i, ch;
    char *file, *endptr;
    TagLib_File *f;
    struct stat s;

    if (argc < 2)
        usage();

    /* tagutil has side effect (like modifying file's properties) so if we
        detect an error in options, we err to end the program. */
    while ((ch = getopt(argc, argv, "edhNYt:r:x:a:A:c:g:y:T:")) != -1) {
        switch ((char)ch) {
        case 'e':
            w = eflag = true;
            break;
        case 'r':
            if (rflag)
                errx(EINVAL, "-r option set twice");
            rflag = true;
            r_arg  = optarg;
            break;
        case 'x':
            if (xflag)
                errx(EINVAL, "-x option set twice");
            xflag = true;
            x_arg  = parse_filter(new_lexer(optarg));
            break;
        case 'a':
            if (aflag)
                errx(EINVAL, "-a option set twice");
            w = true;
            aflag = true;
            a_arg  = optarg;
            break;
        case 'A':
            if (Aflag)
                errx(EINVAL, "-A option set twice");
            w = true;
            Aflag = true;
            A_arg  = optarg;
            break;
        case 'c':
            if (cflag)
                errx(EINVAL, "-c option set twice");
            w = true;
            cflag = true;
            c_arg  = optarg;
            break;
        case 'g':
            if (gflag)
                errx(EINVAL, "-g option set twice");
            w = true;
            gflag = true;
            g_arg  = optarg;
            break;
        case 'y':
            if (yflag)
                errx(EINVAL, "-y option set twice");
            w = true;
            yflag = true;
            y_arg  = (int)strtoul(optarg, &endptr, 10);
            if (endptr == optarg || *endptr != '\0' || y_arg < 0)
                errx(EINVAL, "Invalid year argument: '%s'", optarg);
            break;
        case 'T':
            if (Tflag)
                errx(EINVAL, "-T option set twice");
            w = true;
            Tflag = true;
            T_arg  = (int)strtoul(optarg, &endptr, 10);
            if (endptr == optarg || *endptr != '\0' || T_arg < 0)
                errx(EINVAL, "Invalid track argument: '%s'", optarg);
            break;
        case 't':
            if (tflag)
                errx(EINVAL, "-t option set twice");
            w = true;
            tflag = true;
            t_arg  = optarg;
            break;
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

    if (argc == 0)
        errx(EINVAL, "No file argument given, run `%s -h' to see help.",
                getprogname());
    if (dflag && !rflag)
        errx(EINVAL, "-d is only valid with -r.");
    if (xflag && (w || rflag))
        errx(EINVAL, "-x option must be used alone");
    if (!xflag && !rflag && !w)
    /* no action given, fallback to default */
        pflag = true;

    /* taglib specific init */
    taglib_set_strings_unicode(has_match(getenv("LC_ALL"), "utf-?8"));
    taglib_set_string_management_enabled(true);

    for (i = 0; i < argc; i++) {
        file = argv[i];

        if (stat(file, &s) != 0) {
            warn("%s", file);
            continue;
        }
        else if (!S_ISREG(s.st_mode)) {
            warnx("%s is not a regular file", file);
            continue;
        }

        f = taglib_file_new(file);
        if (f == NULL || !taglib_file_is_valid(f)) {
            warnx("%s is not a valid music file", file);
            continue;
        }

        /* modifiy tag, edit, rename */
        if (pflag)
            tagutil_print(file, f);
        else if (xflag)
            tagutil_filter(file, f, x_arg);
        else {
            if (tflag)
                tagutil_title(f, t_arg);
            if (aflag)
                tagutil_album(f, a_arg);
            if (Aflag)
                tagutil_artist(f, A_arg);
            if (yflag)
                tagutil_year(f, y_arg);
            if (Tflag)
                tagutil_track(f, T_arg);
            if (cflag)
                tagutil_comment(f, c_arg);
            if (gflag)
                tagutil_genre(f, g_arg);
            if (eflag)
                tagutil_edit(file, f);
            if (w) {
                if (!taglib_file_save(f))
                    err(errno, "couldn't save file '%s'", file);
            }
            if (rflag)
                tagutil_rename(file, f, r_arg);
        }

        taglib_tag_free_strings();
        taglib_file_free(f);
    }

    if (xflag)
        destroy_ast(x_arg);

    return (EXIT_SUCCESS);
}


void
usage(void)
{

    (void)fprintf(stderr, "tagutil v"VERSION "\n\n");
    (void)fprintf(stderr, "usage: %s [OPTION]... [FILE]...\n", getprogname());
    (void)fprintf(stderr, "Modify or display music file's tag.\n");
    (void)fprintf(stderr, "\n");
    (void)fprintf(stderr, "Options:\n");
    (void)fprintf(stderr, "  -h              show this help\n");
    (void)fprintf(stderr, "  -e              show tag and prompt for editing (need $EDITOR environment variable)\n");
    (void)fprintf(stderr, "  -Y              answer yes to all questions\n");
    (void)fprintf(stderr, "  -N              answer no  to all questions\n");
    (void)fprintf(stderr, "  -r [-d] PATTERN rename files with the given PATTERN. you can use keywords in PATTERN:\n");
    (void)fprintf(stderr, "                  title(%s), album(%s), artist(%s), year(%s), track(%s), comment(%s),\n",
                                             kTITLE,    kALBUM,    kARTIST,    kYEAR,    kTRACK,    kCOMMENT);
    (void)fprintf(stderr, "                  and genre(%s). example: \"%s - %s - (%s) - %s\"\n",
                                             kGENRE,              kARTIST, kALBUM, kTRACK, kTITLE);
    (void)fprintf(stderr, "  -x FILTER       print files in matching FILTER\n");
    (void)fprintf(stderr, "  -A ARTIST       update artist tag to ARTIST for all given files\n");
    (void)fprintf(stderr, "  -a ALBUM        update album tag to ALBUM for all given files\n");
    (void)fprintf(stderr, "  -c COMMENT      update comment tag to COMMENT for all given files\n");
    (void)fprintf(stderr, "  -T TRACK        update track tag to TRACK for all given files\n");
    (void)fprintf(stderr, "  -t TITLE        update title tag to TITLE for all given files\n");
    (void)fprintf(stderr, "  -g GENRE        update genre tag to GENRE for all given files\n");
    (void)fprintf(stderr, "  -y YEAR         update year tag to YEAR for all given files\n");
    (void)fprintf(stderr, "\n");

    exit(EXIT_SUCCESS);
}


char *
create_tmpfile(void)
{
    char *tmpdir, *tmpf;

    tmpdir = getenv("TMPDIR");
    if (tmpdir == NULL)
        tmpdir = "/tmp";

    (void)xasprintf(&tmpf, "%s/%s-XXXXXX", tmpdir, getprogname());

    if (mkstemp(tmpf) == -1)
        err(errno, "can't create '%s' file", tmpf);

    return (tmpf);
}


bool
user_edit(const char *restrict path)
{
    int error;
    char *editor, *editcmd;

    assert_not_null(path);

    editor = getenv("EDITOR");
    if (editor == NULL)
        errx(-1, "please, set the $EDITOR environment variable.");
    else if (has_match(editor, "x?emacs"))
        /*
         * we're actually so cool, that we keep the user waiting if $EDITOR
         * start slowly. The slow-editor-detection-algorithm used maybe not
         * the best known at the time of writing, but it has shown really good
         * results and is pretty short and clear.
         */
        (void)fprintf(stderr, "Starting %s. please wait...\n", editor);

    (void)xasprintf(&editcmd, "%s '%s'", editor, path);

    /* test if the shell is avaiable */
    if (system(NULL) == 0)
            err(errno, "can't access shell");

    error = system(editcmd);

    free(editcmd);
    return (error == 0);
}


bool
tagutil_print(const char *restrict path, TagLib_File *restrict f)
{
    char *infos;

    assert_not_null(path);
    assert_not_null(f);

    infos = tags_to_yaml(path, taglib_file_tag(f));
    (void)printf("%s\n", infos);

    free(infos);
    return (true);
}


bool
tagutil_edit(const char *restrict path, TagLib_File *restrict f)
{
    char *tmp_file, *infos;
    FILE *stream;

    assert_not_null(path);
    assert_not_null(f);

    infos = tags_to_yaml(path, taglib_file_tag(f));
    (void)printf("%s\n", infos);

    if (yesno("edit this file")) {
        tmp_file = create_tmpfile();

        stream = xfopen(tmp_file, "w");
        (void)fprintf(stream, "%s", infos);
        xfclose(stream);

        if (!user_edit(tmp_file)) {
            free(infos);
            remove(tmp_file);
            return (false);
        }

        stream = xfopen(tmp_file, "r");
        if (!yaml_to_tags(taglib_file_tag(f), stream))
            warnx("file '%s' not saved.", path);
        else {
            if (!taglib_file_save(f))
                err(errno, "can't save file '%s'", path);
        }

        xfclose(stream);
        /* FIXME: get remove int status */
        remove(tmp_file);
        free(tmp_file);
    }

    free(infos);
    return (true);
}

bool
tagutil_rename(const char *restrict path, TagLib_File *restrict f,
        const char *restrict pattern)
{
    char *ext, *result, *dirn, *fname, *question;

    assert_not_null(path);
    assert_not_null(f);
    assert_not_null(pattern);

    if (strlen(pattern) == 0)
        errx(EINVAL, "wrong rename pattern: '%s'", pattern);

    ext = strrchr(path, '.');
    if (ext == NULL)
        errx(-1, "can't find file extension: '%s'", path);
    ext++; /* skip dot */
    fname = eval_tag(pattern, taglib_file_tag(f));

    /* fname is now OK. store into result the full new path.  */
    dirn = xdirname(path);
    /* add the directory to result if needed */
    if (strcmp(dirn, ".") != 0)
        (void)xasprintf(&result, "%s/%s.%s", dirn, fname, ext);
    else
        (void)xasprintf(&result, "%s.%s", fname, ext);
    free(fname);
    free(dirn);

    /* ask user for confirmation and rename if user want to */
    if (strcmp(path, result) != 0) {
        (void)xasprintf(&question, "rename '%s' to '%s'", path, result);
        if (yesno(question))
            safe_rename(dflag, path, result);
        free(question);
    }

    free(result);
    return (true);
}


bool
tagutil_filter(const char *restrict path, TagLib_File *restrict f,
        const struct ast *restrict ast)
{
    bool ret;

    assert_not_null(path);
    assert_not_null(f);
    assert_not_null(ast);

    ret = eval(path, taglib_file_tag(f), ast);

    if (ret)
        (void)printf("%s\n", path);

    return (ret);
}


bool
tagutil_title(TagLib_File *restrict f, const char *restrict title)
{

    assert_not_null(f);
    assert_not_null(title);

    taglib_tag_set_title(taglib_file_tag(f), title);
    return (true);
}


bool
tagutil_album(TagLib_File *restrict f, const char *restrict album)
{

    assert_not_null(f);
    assert_not_null(album);

    taglib_tag_set_album(taglib_file_tag(f), album);
    return (true);
}


bool
tagutil_artist(TagLib_File *restrict f, const char *restrict artist)
{
    assert_not_null(f);
    assert_not_null(artist);

    taglib_tag_set_artist(taglib_file_tag(f), artist);
    return (true);
}


bool
tagutil_year(TagLib_File *restrict f, int year)
{

    assert_not_null(f);
    assert(year > 0);

    taglib_tag_set_year(taglib_file_tag(f), year);
    return (true);
}


bool
tagutil_track(TagLib_File *restrict f, int track)
{

    assert_not_null(f);
    assert(track > 0);

    taglib_tag_set_track(taglib_file_tag(f), track);
    return (true);
}


bool
tagutil_comment(TagLib_File *restrict f, const char *restrict comment)
{

    assert_not_null(f);
    assert_not_null(comment);

    taglib_tag_set_comment(taglib_file_tag(f), comment);
    return (true);
}


bool
tagutil_genre(TagLib_File *restrict f, const char *restrict genre)
{

    assert_not_null(f);
    assert_not_null(genre);

    taglib_tag_set_genre(taglib_file_tag(f), genre);
    return (true);
}

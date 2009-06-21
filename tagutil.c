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
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <unistd.h> /* getopt(3) */

#include "t_config.h"
#include "t_toolkit.h"
#include "t_tag.h"
#include "t_file.h"
#include "t_ftflac.h"
#include "t_ftoggvorbis.h"
#include "t_ftgeneric.h"
#include "t_yaml.h"
#include "t_renamer.h"
#include "t_lexer.h"
#include "t_parser.h"
#include "t_filter.h"
#include "tagutil.h"


bool pflag = false; /* display tags action */
bool Yflag = false; /* yes answer to all questions */
bool Nflag = false; /* no  answer to all questions */
bool bflag = false; /* show backend */
bool eflag = false; /* edit */
bool dflag = false; /* create directory with rename */
bool rflag = false;  /* rename */
bool xflag = false;  /* filter */
bool sflag = false;  /* set tags */
bool fflag = false;  /* load file */

char *f_arg = NULL; /* file */
struct t_token **r_arg = NULL; /* rename pattern (compiled) */
struct t_ast *x_arg = NULL; /* filter code */
struct t_taglist *s_arg = NULL; /* key=val tags */


/*
 * get action with getopt(3) and then apply it to all files given in argument.
 * usage() is called if an error is detected.
 */
int
main(int argc, char *argv[])
{
    int i, ch, ret;
    char *path, *set_value, *set_key;
    struct stat s;
    struct t_file *file;

    /* tagutil has side effect (like modifying file's properties) so if we
        detect an error in options, we err to end the program. */
    while ((ch = getopt(argc, argv, "abedhNYf:r:x:s:")) != -1) {
        switch ((char)ch) {
        case 'a': /* secret undocumented option */
            (void)printf("The Answer is 42\n");
            exit(EXIT_SUCCESS);
            /* NOTREACHED */
        case 'b':
            bflag = true;
            break;
        case 'e':
            eflag = true;
            break;
        case 'f':
            fflag = true;
            f_arg = optarg;
            break;
        case 'r':
            if (rflag)
                errx(EINVAL, "-r option set twice");
            rflag = true;
            if (t_strempty(optarg))
                errx(EINVAL, "empty rename pattern");
            r_arg = t_rename_parse(optarg);
            break;
        case 'x':
            if (xflag)
                errx(EINVAL, "-x option set twice");
            xflag = true;
            x_arg = t_parse_filter(t_lexer_new(optarg));
            break;
        case 's':
            sflag = true;
            if (s_arg == NULL)
                s_arg = t_taglist_new();
            set_key = optarg;
            set_value = strchr(set_key, '=');
            if (set_value == NULL)
                errx(EINVAL, "`%s': invalid -s argument (no equal)", set_key);
            *set_value = '\0';
            set_value += 1;
            if (t_strempty(set_value))
                errx(EINVAL, "`%s': invalid -s argument (empty value)", set_key);
            t_taglist_insert(s_arg, set_key, set_value);
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

    if (argc == 0) {
        errx(EINVAL, "missing file argument.\nrun `%s -h' to see help.",
                getprogname());
    }
    if (dflag && !rflag)
        errx(EINVAL, "-d is only valid with -r");
    i  = ((bflag || sflag || eflag || rflag) ? 1 : 0);
    i += (xflag ? 1 : 0);
    i += (fflag ? 1 : 0);
    if (i > 1)
        errx(EINVAL, "-x and/or -f option must be used alone");
    if (!bflag && !xflag && !fflag && !rflag && !sflag && !eflag)
    /* no action given, fallback to default */
        pflag = true;

    /* init backends */
    t_ftflac_init();
    t_ftoggvorbis_init();
    t_ftgeneric_init();

    ret = EXIT_SUCCESS;
    for (i = 0; i < argc; i++) {
        path = argv[i];

        if (stat(path, &s) != 0) {
            warn("%s", path);
            ret = EINVAL;
            continue;
        }
        else if (!S_ISREG(s.st_mode)) {
            warnx("`%s' is not a regular file", path);
            ret = EINVAL;
            continue;
        }

        file = NULL;
        file = t_ftflac_new(path);
        if (file == NULL)
            file = t_ftoggvorbis_new(path);
        if (file == NULL)
            file = t_ftgeneric_new(path);
        if (file == NULL) {
            warnx("`%s' unsuported file format", path);
            ret = EINVAL;
            continue;
        }

        /* modifiy tag, edit, rename */
        if (bflag)
            (void)printf("%s: %s\n", file->path, file->lib);
        if (pflag)
            tagutil_print(file);
        if (xflag)
            tagutil_filter(file, x_arg);
        if (fflag)
            tagutil_load(file, f_arg);
        if (sflag) {
            if (!file->clear(file, s_arg) || !file->add(file, s_arg))
                warnx("file `%s' not saved: %s", file->path, t_error_msg(file));
            else if (!file->save(file)) {
                    errx(errno ? errno : -1, "couldn't save file `%s': %s",
                            path, t_error_msg(file));
            }
        }
        if (eflag)
            tagutil_edit(file);
        if (rflag)
            tagutil_rename(file, r_arg);

        file->destroy(file);
    }

    if (xflag)
        t_ast_destroy(x_arg);
    if (sflag)
        t_taglist_destroy(s_arg);
    if (rflag) {
        for (i = 0; r_arg[i]; i++)
            freex(r_arg[i]);
        freex(r_arg);
    }

    return (ret);
}


void
usage(void)
{

    (void)fprintf(stderr, "tagutil v"T_TAGUTIL_VERSION "\n\n");
    (void)fprintf(stderr, "usage: %s [OPTION]... [FILE]...\n", getprogname());
    (void)fprintf(stderr, "Modify or display music file's tag.\n");
    (void)fprintf(stderr, "\n");
    (void)fprintf(stderr, "Backend:\n");
#if defined(WITH_FLAC)
    (void)fprintf(stderr, "  libFLAC:   flac files format, use `Vorbis comment' metadata tags.\n");
#endif
#if defined(WITH_OGGVORBIS)
    (void)fprintf(stderr, "  libvorbis: Ogg/Vorbis files format, use `Vorbis comment' metadata tags.\n");
#endif
#if defined(WITH_TAGLIB)
    (void)fprintf(stderr, "  TagLib:    multiple file format (flac,ogg,mp3...), can handle only a limited set of tags.\n");
#endif
    (void)fprintf(stderr, "\n");
    (void)fprintf(stderr, "Options:\n");
    (void)fprintf(stderr, "  -h              show this help\n");
    (void)fprintf(stderr, "  -b              display backend used for each files\n");
    (void)fprintf(stderr, "  -Y              answer yes to all questions\n");
    (void)fprintf(stderr, "  -N              answer no  to all questions\n");
    (void)fprintf(stderr, "  -e              show tag and prompt for editing (need $EDITOR environment variable)\n");
    (void)fprintf(stderr, "  -f PATH         load PATH yaml file in given music files.\n");
    (void)fprintf(stderr, "  -x FILTER       print files in matching FILTER\n");
    (void)fprintf(stderr, "  -s TAG=VALUE    update tag TAG to VALUE for all given files\n");
    (void)fprintf(stderr, "  -r [-d] PATTERN rename files with the given PATTERN. you can use keywords in PATTERN:\n");
    (void)fprintf(stderr, "                  %%tag if tag contains only `_', `-' or alphanum characters. %%{tag} otherwise.\n");
    (void)fprintf(stderr, "\n");

    exit(EXIT_SUCCESS);
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
    else if (strcmp(editor, "emacs") == 0)
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

    /* FIXME: fork(2) - wait(2) ? */
    error = system(editcmd);

    freex(editcmd);
    return (error == 0);
}


bool
tagutil_print(struct t_file *restrict file)
{
    char *yaml;

    assert_not_null(file);

    yaml = t_tags2yaml(file);
    if (yaml)
        (void)printf("%s\n", yaml);
    else
        warnx("%s", t_error_msg(file));

    freex(yaml);
    return (true);
}


bool
tagutil_load(struct t_file *restrict file, const char *restrict path)
{
    FILE *stream;
    bool ret = true;
    struct t_taglist *T;

    assert_not_null(file);
    assert_not_null(path);

    if (strcmp(path, "-") == 0)
        stream = stdin;
    else
        stream = xfopen(path, "r");

    T = t_yaml2tags(file, stream);
    if (T == NULL) {
        ret = false;
        warnx("error while reading YAML: %s", t_error_msg(file));
        warnx("file `%s' not saved.", file->path);
    }
    else {
        if (!file->clear(file, NULL) || !file->add(file, T))
            warnx("file `%s' not saved: %s", file->path, t_error_msg(file));
        else {
            if (!file->save(file))
                err(errno, "can't save file '%s'", file->path);
        }
        t_taglist_destroy(T);
    }
    if (stream != stdin)
        xfclose(stream);

    return (ret);
}


bool
tagutil_edit(struct t_file *restrict file)
{
    FILE *stream;
    bool ret = true;
    char *tmp_file, *yaml, *editor;

    assert_not_null(file);

    yaml = t_tags2yaml(file);
    if (yaml == NULL) {
        warnx("%s", t_error_msg(file));
        return (false);
    }

    (void)printf("%s\n", yaml);

    if (t_yesno("edit this file")) {
        tmp_file = t_mkstemp("/tmp");

        stream = xfopen(tmp_file, "w");
        (void)fprintf(stream, "%s", yaml);
        editor = getenv("EDITOR");
        if (editor && strcmp(editor, "vim") == 0)
            (void)fprintf(stream, "\n# vim:filetype=yaml");
        xfclose(stream);

        if (!user_edit(tmp_file))
            ret = false;
        else
            ret = tagutil_load(file, tmp_file);

        xunlink(tmp_file);
        freex(tmp_file);
    }

    freex(yaml);
    return (ret);
}


bool
tagutil_rename(struct t_file *restrict file, struct t_token **restrict tknary)
{
    char *ext, *result, *dirn, *fname, *question;
    struct t_error *e;

    assert_not_null(file);
    assert_not_null(tknary);

    ext = strrchr(file->path, '.');
    if (ext == NULL) {
        warnx("can't find file extension for `%s'", file->path);
        return (false);
    }
    ext++; /* skip dot */
    fname = t_rename_eval(file, tknary);
    if (fname == NULL) {
        warnx("%s", t_error_msg(file));
        return (false);
    }

    /* fname is now OK. store into result the full new path.  */
    dirn = t_dirname(file->path);
    if (dirn == NULL)
        err(errno, "%s", dirn);
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
        if (t_yesno(question)) {
            e = t_error_new();
            if (!t_rename_safe(file->path, result, e))
                err(errno, "%s", t_error_msg(e));
            t_error_destroy(e);
        }
        freex(question);
    }

    freex(result);
    return (true);
}


bool
tagutil_filter(struct t_file *restrict file,
        const struct t_ast *restrict ast)
{
    bool ret;

    assert_not_null(file);
    assert_not_null(ast);

    ret = t_filter_eval(file, ast);

    if (ret)
        (void)printf("%s\n", file->path);

    return (ret);
}


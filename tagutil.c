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

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <unistd.h> /* getopt(3) */

#include <taglib/tag_c.h>

#include "t_config.h"
#include "t_lexer.h"
#include "t_parser.h"
#include "t_interpreter.h"
#include "t_toolkit.h"

#include "tagutil.h"


/*
 * get action with getopt(3) and then apply it to all files given in argument.
 * usage() is called if an error is detected.
 */
int
main(int argc, char *argv[])
{
    int i, ch;
    char *current_filename;
    TagLib_File *f;
    tagutil_f apply;
    void *apply_arg;
    struct stat current_filestat;

    apply       = NULL;
    apply_arg   = NULL;

    if (argc < 2)
        usage();

    /* tagutil has side effect (like modifying file's properties) so if we
        detect an error in options, we err to end the program. */
    while ((ch = getopt(argc, argv, "epht:r:x:a:A:c:g:y:T:")) != -1) {
        switch ((char)ch) {
        case 'e':
            if (apply != NULL)
                errx(-1, "too much options given.");
            apply = tagutil_edit;
            break;
        case 'p':
            if (apply != NULL)
                errx(-1, "too much options given.");
            apply = tagutil_print;
            break;
        case 't':
            if (apply != NULL)
                errx(-1, "too much options given.");
            apply = tagutil_title;
            apply_arg = optarg;
            break;
        case 'r':
            if (apply != NULL)
                errx(-1, "too much options given.");
            apply = tagutil_rename;
            apply_arg = optarg;
            break;
        case 'x':
            if (apply != NULL)
                errx(-1, "too much options given.");
            apply = tagutil_filter;
            apply_arg = parse_filter(new_lexer(optarg));
            break;
        case 'a':
            if (apply != NULL)
                errx(-1, "too much options given.");
            apply = tagutil_album;
            apply_arg = optarg;
            break;
        case 'A':
            if (apply != NULL)
                errx(-1, "too much options given.");
            apply = tagutil_artist;
            apply_arg = optarg;
            break;
        case 'c':
            if (apply != NULL)
                errx(-1, "too much options given.");
            apply = tagutil_comment;
            apply_arg = optarg;
            break;
        case 'g':
            if (apply != NULL)
                errx(-1, "too much options given.");
            apply = tagutil_genre;
            apply_arg = optarg;
            break;
        case 'y':
            if (apply != NULL)
                errx(-1, "too much options given.");
            if (atoi(optarg) <= 0)
                errx(-1, "Invalid year argument: %s", optarg);
            apply = tagutil_year;
            apply_arg = optarg;
            break;
        case 'T':
            if (apply != NULL)
                errx(-1, "too much options given.");
            if (atoi(optarg) <= 0)
                errx(-1, "Invalid track argument: %s", optarg);
            apply = tagutil_track;
            apply_arg = optarg;
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
        errx(-1, "No file argument given, run `%s -h' to see help.", getprogname());

    if (apply == NULL) /* no action given, fallback to default */
        apply = tagutil_print;

    /* taglib specific init */
    taglib_set_strings_unicode(has_match(getenv("LC_ALL"), "utf-?8"));
    taglib_set_string_management_enabled(true);

    /*
     * iter through all files arguments
     */
    for (i = 0; i < argc; i++) {
        current_filename = argv[i];

        /* we have to check first if the file exist and if it's a "regular file" */
        if (stat(current_filename, &current_filestat) != 0) {
            warn("%s", current_filename);
            continue;
        }
        else if (!S_ISREG(current_filestat.st_mode)) {
            warnx("%s is not a regular file", current_filename);
            continue;
        }

        if ((f = taglib_file_new(current_filename)) == NULL) {
            warnx("%s is not a music file", current_filename);
            continue;
        }

        (void)(*apply)(current_filename, f, apply_arg);

        taglib_tag_free_strings();
        taglib_file_free(f);
    }

    if (apply == tagutil_filter)
        destroy_ast((struct ast *)apply_arg);

    return (EXIT_SUCCESS);
}


void
usage(void)
{

    (void)fprintf(stderr,  "TagUtil v"__TAGUTIL_VERSION__" by "__TAGUTIL_AUTHOR__".\n\n");
    (void)fprintf(stderr,  "usage: %s [opt [optarg]] [files]...\n", getprogname());
    (void)fprintf(stderr, "Modify or display music file's tag.\n");
    (void)fprintf(stderr, "\n");
    (void)fprintf(stderr, "Options:\n");
    (void)fprintf(stderr, "    -h                     : show this help\n");
    (void)fprintf(stderr, "    -p [files]             : only show files tag. -p is not needed but useful if the first file\n");
    (void)fprintf(stderr, "                             argument may be a file that could match an option.\n");
    (void)fprintf(stderr, "    -e [files]             : show tag and prompt for editing (need $EDITOR environment variable)\n");
    (void)fprintf(stderr, "    -r [PATTERN] [files]   : rename files with the given PATTERN. you can use keywords in PATTERN:\n");
    (void)fprintf(stderr, "                             title(%s), album(%s), artist(%s), year(%s), track(%s), comment(%s),\n",
                                                         kTITLE,    kALBUM,    kARTIST,    kYEAR,    kTRACK,    kCOMMENT);
    (void)fprintf(stderr, "                             and genre(%s). example: \"%s - %s - (%s) - %s\"\n",
                                                             kGENRE,               kARTIST, kALBUM, kTRACK, kTITLE);
    (void)fprintf(stderr, "    -x [FILTER]  [files]   : print file in given files that match given FILTER\n");
    (void)fprintf(stderr, "    -t [TITLE]   [files]   : change title tag to TITLE for all given files\n");
    (void)fprintf(stderr, "    -a [ALBUM]   [files]   : change album tag to ALBUM for all given files\n");
    (void)fprintf(stderr, "    -A [ARTIST]  [files]   : change artist tag to ARTIST for all given files\n");
    (void)fprintf(stderr, "    -y [YEAR]    [files]   : change year tag to YEAR for all given files\n");
    (void)fprintf(stderr, "    -T [TRACK]   [files]   : change track tag to TRACK for all given files\n");
    (void)fprintf(stderr, "    -c [COMMENT] [files]   : change comment tag to COMMENT for all given files\n");
    (void)fprintf(stderr, "    -g [GENRE]   [files]   : change genre tag to GENRE for all given files\n");
    (void)fprintf(stderr, "\n");

    exit(EXIT_SUCCESS);
}


void
safe_rename(const char *restrict oldpath, const char *restrict newpath)
{
    struct stat st;
    char *olddirn, *newdirn;

    assert_not_null(oldpath);
    assert_not_null(newpath);

    olddirn = xdirname(oldpath);
    newdirn = xdirname(newpath);
    if (strcmp(olddirn, newdirn) != 0) {
    /* srcdir != destdir, we need to check if destdir is OK */
        if (stat(newpath, &st) == 0 && S_ISDIR(st.st_mode))
            /* destdir exists, so let's try */;
        else {
            err(errno, "can't rename \"%s\" to \"%s\" destination directory doesn't exist",
                    oldpath, newpath);
        }
    }
    free(olddirn);
    free(newdirn);

    if (stat(newpath, &st) == 0)
        err(errno = EEXIST, "%s", newpath);

    if (rename(oldpath, newpath) == -1)
        err(errno, NULL);
}


char *
create_tmpfile(void)
{
    size_t tmpf_size;
    char *tmpdir, *tmpf;

    tmpf_size = 1;
    tmpf = xcalloc(tmpf_size, sizeof(char));

    tmpdir = getenv("TMPDIR");
    if (tmpdir == NULL)
        tmpdir = "/tmp";

    concat(&tmpf, &tmpf_size, tmpdir);
    concat(&tmpf, &tmpf_size, "/");
    concat(&tmpf, &tmpf_size, getprogname());
    concat(&tmpf, &tmpf_size, "XXXXXX");

    if (mkstemp(tmpf) == -1)
        err(errno, "can't create %s file", tmpf);

    return (tmpf);
}


char *
printable_tag(const TagLib_Tag *restrict tag)
{
    char *template;

    /* mh. don't change this layout, or change also update_tag */
    template =  "title   - \""kTITLE"\"\n"
                "album   - \""kALBUM"\"\n"
                "artist  - \""kARTIST"\"\n"
                "year    - \""kYEAR"\"\n"
                "track   - \""kTRACK"\"\n"
                "comment - \""kCOMMENT"\"\n"
                "genre   - \""kGENRE"\"";

    return (eval_tag(template, tag));
}


bool
user_edit(const char *restrict path)
{
    int error;
    size_t editcmd_size;
    char *editor, *editcmd;

    assert_not_null(path);

    editcmd_size = 1;
    editcmd = xcalloc(editcmd_size, sizeof(char));
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

    concat(&editcmd, &editcmd_size, editor);
    /* ok we really should not have a broken path (with space). but just in
        case, we add the "'s */
    concat(&editcmd, &editcmd_size, " \"");
    concat(&editcmd, &editcmd_size, path);
    concat(&editcmd, &editcmd_size, "\"");

    /* test if the shell is avaiable */
    if (system(NULL) == 0)
            err(errno, "can't access shell");

    error = system(editcmd);

    free(editcmd);
    return (error == 0);
}


bool
update_tag(TagLib_Tag *restrict tag, FILE *restrict stream)
{
    bool ret;
    size_t size;
    char *line, *key, *val;

    ret = true;

    size = BUFSIZ;
    line = xcalloc(size, sizeof(char));

    while (xgetline(&line, &size, stream)) {
        if (!empty_line(line) && !has_match(line, "^[ \t]*#")) {
            /* XXX: use only re_format(7) regexp (no \d,\s,\w for example) */
            if (has_match(line, "^(title|album|artist|year|track|comment|genre) *- *\".*\"$")) {
                key = get_match(line, "^(title|album|artist|year|track|comment|genre)");
                val = get_match(line, "\".*\"$");
                val[strlen(val) - 1] = '\0'; /* trim the ending " */
                val++; /* glup. we jump over the first char " that's why we free(val - 1) below. */

/* yet another sexy macro. */
#define _CALL_SETTER_IF_MATCH(what, new_value)          \
    do {                                                \
        if (strcmp(key, #what) == 0) {                  \
            (taglib_tag_set_ ## what)(tag, new_value);  \
            goto cleanup;                               \
        }                                               \
    } while (/*CONSTCOND*/0)

                _CALL_SETTER_IF_MATCH(title,     val);
                _CALL_SETTER_IF_MATCH(album,     val);
                _CALL_SETTER_IF_MATCH(artist,    val);
                _CALL_SETTER_IF_MATCH(comment,   val);
                _CALL_SETTER_IF_MATCH(genre,     val);
                _CALL_SETTER_IF_MATCH(year,      atoi(val));
                _CALL_SETTER_IF_MATCH(track,     atoi(val));

                warnx("parser error on line: %s", line);
                ret = false;
cleanup:
                free((val - 1));
                free(key);
            }
            else {
                warnx("parser can't handle this line: %s", line);
                ret = false;
            }
        }
    }

    free(line);
    return (ret);
}


/*
 * replace each tagutil keywords by their value. see k* keywords.
 *
 * This function use the following algorithm:
 * 1) start from the end of pattern.
 * 2) search backward for a keyword that we use (like %a, %t etc.)
 * 3) when a keyword is found, replace it by it's value.
 * 4) go to step 2) until we process all the pattern.
 *
 * this algorithm ensure that we expend all and only the keywords that are
 * in the initial pattern (we don't expend keywords in values. for example in
 * an artist value like "foo %t" we won't expend the %t into title).
 * It also ensure that we can expend the same keyword more than once (for
 * example in a pattern like "%t %t %t"  the title tag is "MyTitle" we expend
 * the pattern into "MyTitle MyTitle MyTitle").
 */
char *
eval_tag(const char *restrict pattern, const TagLib_Tag *restrict tag)
{
    regmatch_t matcher;
    int cursor;
    char *c;
    char *result;
    char buf[5]; /* used to convert track/year tag into string. need to be fixed in year > 9999 */

    assert_not_null(pattern);
    assert_not_null(tag);

    /* init result with pattern */
    result = xstrdup(pattern);

#define _REPLACE_BY_STRING_IF_MATCH(pattern, x)                                     \
    do {                                                                            \
            if (strncmp(c, pattern, strlen(pattern)) == 0) {                        \
                matcher.rm_so = cursor;                                             \
                matcher.rm_eo = cursor + strlen(pattern);                           \
                inplacesub_match(&result, &matcher, (taglib_tag_ ## x)(tag));       \
                /* the following line *is* important. If you don't understand       \
                 * why, try tagutil -r '%%a' <file> with the album tag of           \
                 * file set to "An Album". eval_tag() will replace "%%a" by         \
                 * "%An Album" and then cursor will be on the % if we don't         \
                 * cursor--. Then eval_tag() will expend the %A and we don't        \
                 * want to. so cursor-- prevent this and ensure that the first      \
                 * expension will be safe and not modified.                         \
                 */                                                                 \
                cursor--;                                                           \
                goto next_loop_iter;                                                \
            }                                                                       \
    } while (/*CONSTCOND*/0)
#define _REPLACE_BY_INT_IF_MATCH(pattern, x)                                        \
    do {                                                                            \
            if (strncmp(c, pattern, strlen(pattern)) == 0) {                        \
                matcher.rm_so = cursor;                                             \
                matcher.rm_eo = cursor + strlen(pattern);                           \
                (void)snprintf(buf, len(buf), "%02u", (taglib_tag_ ## x)(tag));     \
                inplacesub_match(&result, &matcher, buf);                           \
                cursor--;                                                           \
                goto next_loop_iter;                                                \
            }                                                                       \
    } while (/*CONSTCOND*/0)

    for(cursor = strlen(pattern) - 2; cursor >= 0; cursor--) {
        c = &result[cursor];
        if (*c == '%') {
            /*TODO: implement kESCAPE %% */
            _REPLACE_BY_STRING_IF_MATCH (kTITLE,   title);
            _REPLACE_BY_STRING_IF_MATCH (kALBUM,   album);
            _REPLACE_BY_STRING_IF_MATCH (kARTIST,  artist);
            _REPLACE_BY_STRING_IF_MATCH (kCOMMENT, comment);
            _REPLACE_BY_STRING_IF_MATCH (kGENRE,   genre);
            _REPLACE_BY_INT_IF_MATCH    (kYEAR,    year);
            _REPLACE_BY_INT_IF_MATCH    (kTRACK,   track);
        }
next_loop_iter:
        /* NOOP */;
    }

    return (result);
}


bool
tagutil_print(const char *restrict path, TagLib_File *restrict f,
        __t__unused const void *restrict arg)
{
    char *infos;

    assert_not_null(path);
    assert_not_null(f);

    infos = printable_tag(taglib_file_tag(f));
    (void)printf("FILE: \"%s\"\n", path);
    (void)printf("%s\n\n", infos);

    free(infos);
    return (true);
}


bool
tagutil_edit(const char *restrict path, TagLib_File *restrict f,
        __t__unused const void *restrict arg)
{
    char *tmp_file, *infos;
    FILE *stream;

    assert_not_null(path);
    assert_not_null(f);

    infos = printable_tag(taglib_file_tag(f));
    (void)printf("FILE: \"%s\"\n", path);
    (void)printf("%s\n\n", infos);

    if (yesno("edit this file")) {
        tmp_file = create_tmpfile();

        stream = xfopen(tmp_file, "w");
        (void)fprintf(stream, "# %s\n\n", path);
        (void)fprintf(stream, "%s\n", infos);
        xfclose(stream);

        if (!user_edit(tmp_file)) {
            free(infos);
            remove(tmp_file);
            return (false);
        }

        stream = xfopen(tmp_file, "r");
        if (!update_tag(taglib_file_tag(f), stream))
            warnx("file '%s' not saved.", path);
        else {
            if (!taglib_file_save(f))
                err(errno, "can't save file %s", path);
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
tagutil_rename(const char *restrict path, TagLib_File *restrict f, const void *restrict arg)
{
    char *ftype;
    const char *strarg;
    TagLib_Tag *tag;
    char *result, *dirn, *new_fname;
    size_t result_size, new_fname_size;

    assert_not_null(path);
    assert_not_null(f);
    assert_not_null(arg);

    strarg = (const char *)arg;

    if (strarg[0] == '\0')
        errx(-1, "wrong rename pattern: %s", strarg);

    tag = taglib_file_tag(f);
    new_fname = eval_tag(strarg, tag);
    /* add ftype to new_fname */
    new_fname_size = strlen(new_fname) + 1;
    if ((ftype = strrchr(path, '.')) == NULL)
        errx(-1, "can't find file extension: %s", path);
    concat(&new_fname, &new_fname_size, ftype);

    /* new_fname is now OK. store into result the full new path.  */
    result_size = 1;
    dirn = xdirname(path);
    result = xcalloc(result_size, sizeof(char));
    /* add the directory to result if needed */
    if (strcmp(dirn, ".") != 0) {
        concat(&result, &result_size, dirn);
        concat(&result, &result_size, "/");
    }
    concat(&result, &result_size, new_fname);

    /* ask user for confirmation and rename if user want to */
    if (strcmp(path, result) != 0) {
        (void)printf("rename \"%s\" to \"%s\" ", path, result);
        if (yesno(NULL))
            safe_rename(path, result);
    }

    free(dirn);
    free(new_fname);
    free(result);
    return (true);
}


bool
tagutil_filter(const char *restrict path, TagLib_File *restrict f, const void *restrict arg)
{
    bool ret;

    assert_not_null(path);
    assert_not_null(f);
    assert_not_null(arg);

    ret = eval(path, taglib_file_tag(f), (struct ast *)arg);

    if (ret)
        (void)printf("%s\n", path);

    return (ret);
}


/*
 * tagutil_f generator.
 */
#define _MAKE_TAGUTIL_FUNC(what, hook)                              \
    bool tagutil_##what (const char *restrict path,                 \
                        TagLib_File *restrict f,                    \
                        const void  *restrict arg)                  \
    {                                                               \
                                                                    \
        const char *str;                                            \
                                                                    \
        assert_not_null(path);                                      \
        assert_not_null(f);                                         \
        assert_not_null(arg);                                       \
                                                                    \
        str = (const char *)arg;                                    \
        (taglib_tag_set_##what)(taglib_file_tag(f), hook(str));     \
    if (!taglib_file_save(f))                                       \
        err(errno, "can't save file %s", path);                     \
    return (true);                                                  \
}

_MAKE_TAGUTIL_FUNC(title,)
_MAKE_TAGUTIL_FUNC(album,)
_MAKE_TAGUTIL_FUNC(artist,)
_MAKE_TAGUTIL_FUNC(year, atoi)
_MAKE_TAGUTIL_FUNC(track, atoi)
_MAKE_TAGUTIL_FUNC(comment,)
_MAKE_TAGUTIL_FUNC(genre,)

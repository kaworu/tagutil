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
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Perrin Alexandre, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <taglib/tag_c.h>

#include "config.h"
#include "t_toolkit.h"
#include "tagutil.h"

/**********************************************************************/

/* the program name */
static char *progname;

/**********************************************************************/

/*
 * parse what to do with the parse_argv function, and then apply it to each
 * file given in argument. usage() is called if an error is detected.
 */
int main(int argc, char *argv[])
{
    int i;
    char *current_filename, *apply_arg;
    TagLib_File *f;
    tagutil_f apply;
    struct stat current_filestat;

    INIT_ALLOC_COUNTER;
    progname = argv[0];

    /* parse_argv will set i, apply_arg, and return the function to use. */
    apply = parse_argv(argc, argv, &i, &apply_arg);

    /* taglib specific init */
    taglib_set_strings_unicode(has_match(getenv("LC_ALL"), "utf-?8"));
    taglib_set_string_management_enabled(true);

    /*
     * iter through all files arguments
     */
    for (; i < argc; i++) {
        current_filename = argv[i];

        /* we have to check first if the file exist and if it's a "regular file" */
        if (stat(current_filename, &current_filestat) != 0)
            die(("can't access %s: ", current_filename));
        if (!S_ISREG(current_filestat.st_mode))
            die(("%s: not a regular file\n", current_filename));


        f = taglib_file_new(current_filename);

        if (f == NULL)
            die(("%s: not a music file\n", current_filename));

        (void) (*apply)(current_filename, f, apply_arg);

        taglib_tag_free_strings();
        taglib_file_free(f);
    }

    return (EXIT_SUCCESS);
}


void
usage()
{

    (void) fprintf(stderr,  "TagUtil v"__TAGUTIL_VERSION__" by "__TAGUTIL_AUTHOR__".\n\n");
    (void) fprintf(stderr,  "usage: %s [opt [optarg]] [files]...\n", progname);
    (void) fprintf(stderr, "Modify or output music file's tag.\n");
    (void) fprintf(stderr, "\n");
    (void) fprintf(stderr, "Options:\n");
    (void) fprintf(stderr, "    -- [files]             : only show files tag. -- is not needed but useful if the first file\n");
    (void) fprintf(stderr, "                             argument may be a file that could match an option.\n");
    (void) fprintf(stderr, "    -e [files]             : show tag and prompt for editing (need $EDITOR environment variable)\n");
    (void) fprintf(stderr, "    -r [PATTERN] [files]   : rename files with the given PATTERN. you can use keywords in PATTERN:\n");
    (void) fprintf(stderr, "                             title(%s), album(%s), artist(%s), year(%s), track(%s), comment(%s),\n",
                                                         kTITLE,    kALBUM,    kARTIST,    kYEAR,    kTRACK,    kCOMMENT);
    (void) fprintf(stderr, "                             and genre(%s). example: \"%s - %s - (%s) - %s\"\n",
                                                             kGENRE,               kARTIST, kALBUM, kTRACK, kTITLE);
    (void) fprintf(stderr, "    -t [TITLE]   [files]   : change title tag to TITLE for all given files\n");
    (void) fprintf(stderr, "    -a [ALBUM]   [files]   : change album tag to ALBUM for all given files\n");
    (void) fprintf(stderr, "    -A [ARTIST]  [files]   : change artist tag to ARTIST for all given files\n");
    (void) fprintf(stderr, "    -y [YEAR]    [files]   : change year tag to YEAR for all given files\n");
    (void) fprintf(stderr, "    -T [TRACK]   [files]   : change track tag to TRACK for all given files\n");
    (void) fprintf(stderr, "    -c [COMMENT] [files]   : change comment tag to COMMENT for all given files\n");
    (void) fprintf(stderr, "    -g [GENRE]   [files]   : change genre tag to GENRE for all given files\n");

    exit(EXIT_SUCCESS);
}


void
safe_rename(const char *__restrict__ oldpath, const char *__restrict__ newpath)
{
    struct stat dummy;

    assert_not_null(oldpath);
    assert_not_null(newpath);

    if (stat(newpath, &dummy) == 0) {
        errno = EEXIST;
        die(("%s: ", newpath));
    }

    if (rename(oldpath, newpath) == -1)
        die(("can't rename \"%s\" to \"%s\"\n", oldpath, newpath));
}


char *
create_tmpfile()
{
    size_t tmpfile_size;
    char *tmpdir, *tmpfile;

    tmpfile_size = 1;
    tmpfile = xcalloc(tmpfile_size, sizeof(char));

    tmpdir = getenv("TMPDIR");
    if (tmpdir == NULL)
        tmpdir = "/tmp";

    concat(&tmpfile, &tmpfile_size, tmpdir);
    concat(&tmpfile, &tmpfile_size, "/");
    concat(&tmpfile, &tmpfile_size, progname);
    concat(&tmpfile, &tmpfile_size, "XXXXXX");

    if (mkstemp(tmpfile) == -1) {
        die(("call to mkstemp failed, can't create %s file\n", tmpfile));
        /* NOTREACHED */
    }
    else
        return (tmpfile);
}


char *
printable_tag(const TagLib_Tag *__restrict__ tag)
{
    char *template;

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
user_edit(const char *__restrict__ path)
{

    int error;
    size_t editcmd_size;
    char *editor, *editcmd;

    assert_not_null(path);

    editcmd_size = 1;
    editcmd = xcalloc(editcmd_size, sizeof(char));
    editor = getenv("EDITOR");
    if (editor == NULL)
        die(("please, set the $EDITOR environment variable.\n"));
    else if (has_match(editor, "x?emacs")) {
        /*
         * we're actually so cool, that we keep the user waiting if $EDITOR
         * start slowly. The slow-editor-detection-algorithm used maybe not
         * the best known at the time of writing, but it has shown really good
         * results and is pretty short and clear.
         */
        (void) fprintf(stderr, "Starting %s. please wait...\n", editor);
    }

    concat(&editcmd, &editcmd_size, editor);
    concat(&editcmd, &editcmd_size, " ");
    concat(&editcmd, &editcmd_size, path);

    /* test if the shell is avaiable */
    if ((system(NULL) == 0))
            die(("can't access shell :(\n"));

    error = system(editcmd);

    free(editcmd);
    return (error == 0 ? true : false);
}


void
update_tag(TagLib_Tag *__restrict__ tag, FILE *__restrict__ fp)
{
    size_t size;
    char *line, *key, *val;

    size = BUFSIZ;
    line = xcalloc(size, sizeof(char));

    while (xgetline(&line, &size, fp)) {
        if (!EMPTY_LINE(line)) {
            if (has_match(line, "^(title|album|artist|year|track|comment|genre)\\s*-\\s*\".*\"$")) {
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

                die(("parser error on line:\n%s\n", line));
cleanup:
                free((val - 1));
                free(key);
            }
            else
                die(("parser can't handle this line :(\n%s\n", line));
        }
    }
    free(line);
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
eval_tag(const char *__restrict__ pattern, const TagLib_Tag *__restrict__ tag)
{
    regmatch_t matcher;
    size_t size;
    int cursor;
    char *c;
    char *result;
    char buf[5]; /* used to convert track/year tag into string */

    assert_not_null(pattern);
    assert_not_null(tag);

    /* init result with pattern */
    size = strlen(pattern) + 1;
    result = xcalloc(size, sizeof(char));
    memcpy(result, pattern, size);

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
                (void) snprintf(buf, len(buf), "%02u", (taglib_tag_ ## x)(tag));    \
                inplacesub_match(&result, &matcher, buf);                           \
                cursor--;                                                           \
                goto next_loop_iter;                                                \
            }                                                                       \
    } while (/*CONSTCOND*/0)

    for(cursor = size - 3; cursor >= 0; cursor--) {
        c = &result[cursor];
        if (*c == '%') {
            _REPLACE_BY_STRING_IF_MATCH (kTITLE,   title);
            _REPLACE_BY_STRING_IF_MATCH (kALBUM,   album);
            _REPLACE_BY_STRING_IF_MATCH (kARTIST,  artist);
            _REPLACE_BY_STRING_IF_MATCH (kCOMMENT, comment);
            _REPLACE_BY_STRING_IF_MATCH (kGENRE,   genre);
            _REPLACE_BY_INT_IF_MATCH    (kYEAR,    year);
            _REPLACE_BY_INT_IF_MATCH    (kTRACK,   track);
        }
next_loop_iter:
        /* NOP */;
    }

    return (result);
}


tagutil_f
parse_argv(int argc, char *argv[], int *first_fname, char **apply_arg)
{
    char *cmd;

    if (argc < 2)
        usage();

    cmd = argv[1];

    if (has_match(cmd, "^-.$")) { /* option */
        if (argc < 3)
            usage();

        *first_fname = 2;
        *apply_arg = NULL;

#define _ON_OPT(opt, func, hook)        \
    do {                                \
        if (strcmp(cmd, opt) == 0) {    \
            hook                        \
            return (tagutil_ ## func);  \
        }                               \
    } while (/*CONSTCOND*/0)

        _ON_OPT("-e", edit, );
        _ON_OPT("--", print, );

        /* other opts than -e need more args. */
        if (argc < 4)
            usage();
        *first_fname = 3;
        *apply_arg = argv[2];

        _ON_OPT("-t", title, );
        _ON_OPT("-r", rename, );
        _ON_OPT("-a", album, );
        _ON_OPT("-A", artist, );
        _ON_OPT("-c", comment, );
        _ON_OPT("-g", genre, );
        _ON_OPT("-y", year,
            if (atoi(*apply_arg) <= 0 && strcmp(*apply_arg, "0") != 0)
                die(("bad year argument: %s\n", *apply_arg));
        );
        _ON_OPT("-T", track,
            if (atoi(*apply_arg) <= 0 && strcmp(*apply_arg, "0") != 0)
                die(("bad track argument: %s\n", *apply_arg));
        );
    }

    /* no option, fallback to print */
    *first_fname = 1;
    *apply_arg = NULL;
    return (tagutil_print);
}


bool
tagutil_print(const char *__restrict__ path, TagLib_File *__restrict__ f,
        const char *__restrict__ arg __attribute__ ((__unused__)))
{
    char *infos;

    assert_not_null(path);
    assert_not_null(f);

    infos = printable_tag(taglib_file_tag(f));
    (void) printf("FILE: \"%s\"\n", path);
    (void) printf("%s\n\n", infos);

    free(infos);
    return (true);
}


bool
tagutil_edit(const char *__restrict__ path, TagLib_File *__restrict__ f,
        const char *__restrict__ arg __attribute__ ((__unused__)))
{
    char *tmp_file, *infos;
    FILE *fp;

    assert_not_null(path);
    assert_not_null(f);

    infos = printable_tag(taglib_file_tag(f));
    (void) printf("FILE: \"%s\"\n", path);
    (void) printf("%s\n\n", infos);

    if (yesno("edit this file")) {
        tmp_file = create_tmpfile();

        fp = xfopen(tmp_file, "w");
        (void) fprintf(fp, "%s\n", infos);
        xfclose(fp);

        if (!user_edit(tmp_file)) {
            free(infos);
            remove(tmp_file);
            return (false);
        }

        fp = xfopen(tmp_file, "r");
        update_tag(taglib_file_tag(f), fp);

        if (!taglib_file_save(f))
            die(("can't save file %s.\n", path));

        xfclose(fp);
        remove(tmp_file);
        free(tmp_file);
    }

    free(infos);
    return (true);
}


bool
tagutil_rename(const char *__restrict__ path, TagLib_File *__restrict__ f, const char *__restrict__ arg)
{
    char *ftype;
    TagLib_Tag *tag;
    char *result, *dirn, *new_fname;
    size_t result_size, new_fname_size;

    assert_not_null(path);
    assert_not_null(f);
    assert_not_null(arg);

    if (arg[0] == '\0')
        die(("wrong rename pattern: %s\n", arg));

    tag = taglib_file_tag(f);
    new_fname = eval_tag(arg, tag);
    /* add ftype to new_fname */
    new_fname_size = strlen(new_fname) + 1;
    if ((ftype = strrchr(path, '.')) == NULL)
        die(("can't find file type: %s\n", path));
    concat(&new_fname, &new_fname_size, ftype);

    /* new_fname is now OK. store into result the full new path.  */
    result_size = 1;
    dirn = xdirname(path);
    result = xcalloc(result_size, sizeof(char));
    if (strcmp(dirn, ".") != 0) {
        concat(&result, &result_size, dirn);
        concat(&result, &result_size, "/");
    }
    concat(&result, &result_size, new_fname);

    /* ask user for confirmation and rename if user want to */
    if (strcmp(path, result) != 0) {
        (void) printf("rename \"%s\" to \"%s\" ", path, result);
        if (yesno(NULL))
            safe_rename(path, result);
    }

    free(dirn);
    free(new_fname);
    free(result);
    return (true);
}


/*
 * tagutil_f generator.
 */
#define _MAKE_TAGUTIL_FUNC(what, hook)                              \
    bool tagutil_##what (const char *__restrict__ path,             \
                        TagLib_File *__restrict__ f,                \
                        const char  *__restrict__ arg)              \
    {                                                               \
        assert_not_null(path);                                      \
        assert_not_null(f);                                         \
        assert_not_null(arg);                                       \
        (taglib_tag_set_##what)(taglib_file_tag(f), hook(arg));     \
    if (!taglib_file_save(f))                                       \
        die(("can't save file %s.\n", path));                       \
    return (true);                                                  \
}

_MAKE_TAGUTIL_FUNC(title,)
_MAKE_TAGUTIL_FUNC(album,)
_MAKE_TAGUTIL_FUNC(artist,)
_MAKE_TAGUTIL_FUNC(year, atoi)
_MAKE_TAGUTIL_FUNC(track, atoi)
_MAKE_TAGUTIL_FUNC(comment,)
_MAKE_TAGUTIL_FUNC(genre,)

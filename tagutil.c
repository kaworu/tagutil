#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <regex.h>
#include <libgen.h> /* for dirname() POSIX.1-2001 */

#include <tag_c.h>

#define __TAGUTIL_VERSION__ "1.0"
#define kTITLE      "%t"
#define kALBUM      "%a"
#define kARTIST     "%A"
#define kYEAR       "%y"
#define kTRACK      "%T"
#define kCOMMENT    "%c"
#define kGENRE      "%g"
/**********************************************************************/

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L /* C99 */
#include <stdbool.h>
#define __inline__ inline
#define __restrict__ restrict
#define __FUNCNAME__ __func__

#else /* !C99 */

typedef unsigned char bool;
#define false (0)
#define true (!false)
#define __inline__
#define __restrict__
#if defined(__GNUC__)
#define __FUNCNAME__ __FUNCTION__
#else /* !__GNUC__ */
#define __FUNCNAME__ ""
#endif /* __GNUC__ */

#endif /* C99 */

/* use gcc attribute when available */
#if !defined(__GNUC__)
#  define __attribute__(x)
#endif


#if !defined(NDEBUG)
#define die(vargs)                              \
    do {                                        \
        (void) fprintf(stderr, "%s at %d:\n",   \
                __FUNCNAME__, __LINE__);        \
        _die vargs;                             \
    } while (/*CONSTCOND*/0)
#define debug(type, expr) (void) fprintf(stderr, #expr " = " type "\n", (expr))
#define INIT_ALLOC_COUNTER      \
    do {                        \
        alloc_counter = 0;      \
        alloc_b = 0;            \
    } while (/*CONSTCOND*/0)
#define INCR_ALLOC_COUNTER(x)   \
    do {                        \
        alloc_counter++;        \
        alloc_b += (x);         \
    } while (/*CONSTCOND*/0)
#else /* NDEBUG */
#define die(vargs) _die vargs
#define debug(type, expr)
#define INIT_ALLOC_COUNTER
#define INCR_ALLOC_COUNTER
#endif /* !NDEBUG */

/* compute the length of a fixed size array */
#define len(ary) (sizeof(ary) / sizeof((ary)[0]))
/* true if the given string is empty (has only whitespace char) */
#define EMPTY_LINE(l) has_match((l), "^\\s*$")

/**********************************************************************/

/* the program name */
static char *progname;
#if !defined(NDEBUG)
/* count how many times we call x*alloc */
static int alloc_counter;
static int alloc_b;
#endif

/*
 * tagutil_f is the type of function that implement actions in tagutil.
 * 1rst arg: the current file's name
 * 2nd  arg: the TagLib_File of the current file
 * 3rd  arg: the arg supplied to the action (for example, if -y is given as
 * option then action is tagutil_year and arg is the new value for the
 * year tag.
 */
typedef bool (*tagutil_f)(const char *__restrict__, TagLib_File *__restrict__, const char *__restrict__);

/**********************************************************************/

/* tools functions */
void usage(void) __attribute__ ((__noreturn__));
void _die(const char *__restrict__ fmt, ...)
    __attribute__ ((__noreturn__, __format__ (__printf__, 1, 2), __nonnull__ (1)));
bool yesno(const char *__restrict__ question);

/* memory functions */
static __inline__ void* xmalloc(const size_t size) __attribute__ ((__malloc__, __unused__));
static __inline__ void* xcalloc(const size_t nmemb, const size_t size)
    __attribute__ ((__malloc__, __unused__));
static __inline__ void* xrealloc(void *old_ptr, const size_t new_size)
    __attribute__ ((__malloc__, __unused__));

/* file functions */
static __inline__ FILE* xfopen(const char *__restrict__ path, const char *__restrict__ mode)
    __attribute__ ((__nonnull__ (1, 2), __unused__));
static __inline__ void  xfclose(FILE *__restrict__ fp)
    __attribute__ ((__nonnull__ (1), __unused__));
static __inline__ bool xgetline(char **line, size_t *size, FILE *__restrict__ fp)
    __attribute__ ((__nonnull__ (1, 2, 3)));
static __inline__ char* xdirname(const char *__restrict__ path)
    __attribute__ ((__nonnull__ (1)));
static __inline__ void safe_rename(const char *__restrict__ oldpath, const char *__restrict__ newpath)
    __attribute__ ((__nonnull__ (1, 2)));
char* create_tmpfile(void);

/* basic string operations */
void concat(char **dest, size_t *dest_size, const char *src) __attribute__ ((__nonnull__ (1, 2, 3)));

/* regex string operations */
regmatch_t* first_match(const char *__restrict__ str, const char *__restrict__ pattern, const int flags)
    __attribute__ ((__nonnull__ (1, 2)));
bool has_match(const char *__restrict__ str, const char *__restrict__ pattern)
    __attribute__ ((__nonnull__ (2)));
char* get_match(const char *__restrict__ str, const char *__restrict__ pattern)
    __attribute__ ((__nonnull__ (1, 2)));
char* sub_match(const char *str, const regmatch_t *__restrict__ match, const char *replace)
    __attribute__ ((__nonnull__ (1, 2, 3)));
void inplacesub_match(char **str, regmatch_t *__restrict__ match, const char *replace)
    __attribute__ ((__nonnull__ (1, 2, 3)));

/* tag functions */
char* printable_tag(const TagLib_Tag *__restrict__ tag)
    __attribute__ ((__nonnull__ (1)));
bool user_edit(const char *__restrict__ path)
    __attribute__ ((__nonnull__ (1)));
void update_tag(TagLib_Tag *__restrict__ tag, FILE *__restrict__ fp)
    __attribute__ ((__nonnull__(1, 2)));
char* eval_tag(const char *__restrict__ pattern, const TagLib_Tag *__restrict__ tag)
    __attribute__ ((__nonnull__(1, 2)));

tagutil_f parse_argv(int argc, char *argv[], int *first_fname, char **apply_arg)
    __attribute__ ((__nonnull__ (2, 3, 4)));
/* tagutil functions */
bool tagutil_print(const char *__restrict__ path, TagLib_File *__restrict__ f, const char *__restrict__ arg)
    __attribute__ ((__nonnull__ (1, 2)));
bool tagutil_edit(const char *__restrict__ path, TagLib_File *__restrict__ f, const char *__restrict__ arg)
    __attribute__ ((__nonnull__ (1, 2)));
bool tagutil_rename(const char *__restrict__ path, TagLib_File *__restrict__ f, const char *__restrict__ arg)
    __attribute__ ((__nonnull__ (1, 2, 3)));
bool tagutil_title(const char *__restrict__ path, TagLib_File *__restrict__ f, const char *__restrict__ arg)
    __attribute__ ((__nonnull__ (1, 2, 3)));
bool tagutil_album(const char *__restrict__ path, TagLib_File *__restrict__ f, const char *__restrict__ arg)
    __attribute__ ((__nonnull__ (1, 2, 3)));
bool tagutil_artist(const char *__restrict__ path, TagLib_File *__restrict__ f, const char *__restrict__ arg)
    __attribute__ ((__nonnull__ (1, 2, 3)));
bool tagutil_year(const char *__restrict__ path, TagLib_File *__restrict__ f, const char *__restrict__ arg)
    __attribute__ ((__nonnull__ (1, 2, 3)));
bool tagutil_track(const char *__restrict__ path, TagLib_File *__restrict__ f, const char *__restrict__ arg)
    __attribute__ ((__nonnull__ (1, 2, 3)));
bool tagutil_comment(const char *__restrict__ path, TagLib_File *__restrict__ f, const char *__restrict__ arg)
    __attribute__ ((__nonnull__ (1, 2, 3)));
bool tagutil_genre(const char *__restrict__ path, TagLib_File *__restrict__ f, const char *__restrict__ arg)
    __attribute__ ((__nonnull__ (1, 2, 3)));

/**********************************************************************/


/*
 * parse what to do with the parse_argv function, and then apply it to each
 * file given in argument. usage() is called if an error is detected.
 */
int main(int argc, char *argv[])
{
    int i;
    char *current_file, *apply_arg;
    TagLib_File *f;
    tagutil_f apply;

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
        current_file = argv[i];
        f = taglib_file_new(current_file);

        if (f == NULL) {
            die(("%s: not a music file\n", current_file));
        }

        (void) (*apply)(current_file, f, apply_arg);

        taglib_tag_free_strings();
        taglib_file_free(f);
    }

    debug("%d", alloc_counter);
    debug("%d", alloc_b);
    return (EXIT_SUCCESS);
}


/*
 * show usage and exit.
 */
void
usage()
{

    (void) fprintf(stderr,  "TagUtil v"__TAGUTIL_VERSION__" by kAworu.\n");
    (void) fprintf(stderr,  "usage: %s <opt> [optarg] <files>\n", progname);
    (void) fprintf(stderr, "Modify or output music file's tag.\n");
    (void) fprintf(stderr, "\n");
    (void) fprintf(stderr, "Options:\n");
    (void) fprintf(stderr, "    -- <files>             : only show files tag. -- is not needed but useful if the first file\n");
    (void) fprintf(stderr, "                             argument may be a file that could match an option.\n");
    (void) fprintf(stderr, "    -e <files>             : show tag and prompt for editing (need $EDITOR environment variable)\n");
    (void) fprintf(stderr, "    -r <PATTERN> <files>   : rename files with the given PATTERN. you can use keywords in PATTERN:\n");
    (void) fprintf(stderr, "                             title(%s), album(%s), artist(%s), year(%s), track(%s), comment(%s),\n",
                                                         kTITLE,    kALBUM,    kARTIST,    kYEAR,    kTRACK,    kCOMMENT);
    (void) fprintf(stderr, "                             and genre(%s). example: \"%s - %s - (%s) - %s\"\n",
                                                             kGENRE,               kARTIST, kALBUM, kTRACK, kTITLE);
    (void) fprintf(stderr, "    -t <TITLE> <files>     : change title tag to TITLE for all given files\n");
    (void) fprintf(stderr, "    -a <ALBUM> <files>     : change album tag to ALBUM for all given files\n");
    (void) fprintf(stderr, "    -A <ARTIST> <files>    : change artist tag to ARTIST for all given files\n");
    (void) fprintf(stderr, "    -y <YEAR> <files>      : change year tag to YEAR for all given files\n");
    (void) fprintf(stderr, "    -T <TRACK> <files>     : change track tag to TRACK for all given files\n");
    (void) fprintf(stderr, "    -c <COMMENT> <files>   : change comment tag to COMMENT for all given files\n");
    (void) fprintf(stderr, "    -g <GENRE> <files>     : change genre tag to GENRE for all given files\n");

    exit(EXIT_SUCCESS);
}


/*
 * print the fmt message, then if errno is set print the errno message and
 * exit with the errno exit status.
 */
void
_die(const char *__restrict__ fmt, ...)
{
    va_list args;

    assert(fmt != NULL);

    va_start(args, fmt);
    (void) vfprintf(stderr, fmt, args);
    va_end(args);

    if (errno != 0)
        perror((char *)NULL);

    exit(errno);
}


/*
 * print the given question, and read user's input. input should match
 * y|yes|n|no.  yesno() loops until a valid response is given and then return
 * true if the response match y|yes, false if it match n|no.
 */
bool
yesno(const char *__restrict__ question)
{
    char buffer[5]; /* strlen("yes\n\0") == 5 */

    for (;;) {
        if (feof(stdin))
            return (false);

        (void) memset(buffer, '\0', len(buffer));

        if (question != NULL)
            (void) printf("%s ", question);
        (void) printf("? [y/n] ");

        (void) fgets(buffer, len(buffer), stdin);

        if (has_match(buffer, "^(n|no)$"))
            return (false);
        if (has_match(buffer, "^(y|yes)$"))
            return (true);

        /* if any, eat stdin characters that didn't fit into buffer */
        if (buffer[strlen(buffer) - 1] != '\n') {
            while (getc(stdin) != '\n' && !feof(stdin))
                ;
        }

    }
}


static __inline__ void *
xmalloc(const size_t size)
{
    void *ptr;

    INCR_ALLOC_COUNTER(size);

    ptr = malloc(size);
    if (size > 0 && ptr == NULL)
        die(("malloc failed.\n"));

    return (ptr);
}


static __inline__ void *
xcalloc(const size_t nmemb, const size_t size)
{
    void *ptr;

    INCR_ALLOC_COUNTER(nmemb * size);

    ptr = calloc(nmemb, size);
    if (ptr == NULL && size > 0 && nmemb > 0)
        die(("calloc failed.\n"));

    return (ptr);
}


static __inline__ void *
xrealloc(void *old_ptr, const size_t new_size)
{
    void *ptr;

    INCR_ALLOC_COUNTER(new_size);

    ptr = realloc(old_ptr, new_size);
    if (ptr == NULL && new_size > 0)
        die(("realloc failed.\n"));

    return (ptr);
}


static __inline__ FILE *
xfopen(const char *__restrict__ path, const char *__restrict__ mode)
{
    FILE *fp;

    assert(path != NULL);
    assert(mode != NULL);

    fp = fopen(path, mode);

    if (fp == NULL)
        die(("can't open file %s\n", path));

    return (fp);
}


static __inline__ void
xfclose(FILE *__restrict__ fp)
{

    assert(fp != NULL);

    if (fclose(fp) != 0)
        die(("xclose failed.\n"));
}


static __inline__ bool
xgetline(char **line, size_t *size, FILE *__restrict__ fp)
{
    char *cursor;
    size_t end;

    assert(fp    != NULL);
    assert(size  != NULL);
    assert(line  != NULL);

    end = 0;

    if (feof(fp))
        return (0);

    for (;;) {
        if (*line == NULL || end > 0) {
            *size += BUFSIZ;
            *line = xrealloc(*line, *size);
        }
        cursor = *line + end;
        memset(*line, 0, BUFSIZ);
        (void) fgets(cursor, BUFSIZ, fp);
        end += strlen(cursor);

        if (feof(fp) || (*line)[end - 1] == '\n') { /* end of file or end of line */
        /* chomp trailing \n if any */
            if ((*line)[0] != '\0' && (*line)[end - 1] == '\n')
                (*line)[end - 1] = '\0';

            return (end);
        }
    }
}


/*
 * try to have a *sane* dirname() function.
 * dirname() implementation may change the
 * char* argument.
 *
 * return value has to be freed.
 */
static __inline__ char *
xdirname(const char *__restrict__ path)
{
    char *pathcpy, *dirn, *dirncpy;
    size_t size;

    assert(path != NULL);

    /* we need to copy path, because dirname() can change the path argument. */
    size = strlen(path) + 1;
    pathcpy = xcalloc(size, sizeof(char));
    memcpy(pathcpy, path, size);

    dirn = dirname(pathcpy);

    /* copy dirname to dirnamecpy */
    size = strlen(dirn) + 1;
    dirncpy = xcalloc(size, sizeof(char));
    memcpy(dirncpy, dirn, size);

    free((void *)pathcpy);

    return (dirncpy);
}


/*
 * rename path to new_path. die() if new_path already exist.
 */
static __inline__ void
safe_rename(const char *__restrict__ oldpath, const char *__restrict__ newpath)
{
    struct stat dummy;

    assert(oldpath != NULL);
    assert(newpath != NULL);

    if (stat(newpath, &dummy) == 0) {
        /*errno = EEXIST;*/
        die(("file \"%s\" already exist.\n", newpath));
    }

    if (rename(oldpath, newpath) == -1)
        die(("can't rename \"%s\" to \"%s\"\n", oldpath, newpath));
}


/*
 * create a temporary file in $TMPDIR. if $TMPDIR is not set, /tmp is
 * used. return the full path to the temp file created.
 *
 * return value has to be freed.
 */
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

    if (mkstemp(tmpfile) == -1)
        die(("call to mkstemp failed, can't create %s file\n", tmpfile));
        /* NOTREACHED */
    else
        return (tmpfile);
}


/*
 * copy src at the end of dest. dest_size is the allocated size
 * of dest and is modified (dest is realloc).
 *
 * return value has to be freed.
 */
void
concat(char **dest, size_t *dest_size, const char *src)
{
    size_t start, src_size, final_size;

    assert(dest         != NULL);
    assert(*dest        != NULL);
    assert(dest_size    != NULL);
    assert(src          != NULL);

    start       = *dest_size - 1;
    src_size    = strlen(src);
    final_size  = *dest_size + src_size;

    *dest = xrealloc(*dest, final_size);

    memcpy(*dest + start, src, src_size + 1);
    *dest_size = final_size;
}


/*
 * compile the given pattern, match it with str and return the regmatch_t result.
 * if an error occure (during the compilation or the match), print the error message
 * and die(). return NULL if there was no match, and a pointer to the regmatch_t
 * otherwise.
 *
 * return value has to be freed.
 */
regmatch_t *
first_match(const char *__restrict__ str, const char *__restrict__ pattern, const int flags)
{
    regex_t regex;
    regmatch_t *regmatch;
    int error;
    char *errbuf;

    assert(str != NULL);
    assert(pattern != NULL);

    regmatch = xmalloc(sizeof(regmatch_t));
    error = regcomp(&regex, pattern, flags);

    if (error != 0)
        goto error;

    error = regexec(&regex, str, 1, regmatch, 0);
    regfree(&regex);

    switch (error) {
    case 0:
        return (regmatch);
    case REG_NOMATCH:
        free(regmatch);
        return ((regmatch_t *)NULL);
    default:
error:
        errbuf = xcalloc(BUFSIZ, sizeof(char));
        (void) regerror(error, &regex, errbuf, BUFSIZ);
        die((errbuf));
        /* NOTREACHED */
    }
}


/*
 * return true if the regex pattern match the given str, false otherwhise.
 * flags are REG_ICASE | REG_EXTENDED | REG_NEWLINE | REG_NOSUB (see regex(3)).
 */
bool
has_match(const char *__restrict__ str, const char *__restrict__ pattern)
{
    regmatch_t *match;

    assert(pattern != NULL);

    if (str == NULL)
        return (false);

    match = first_match(str, pattern, REG_ICASE | REG_EXTENDED | REG_NEWLINE | REG_NOSUB);

    if (match == NULL)
        return (false);
    else {
        free((void *)match);
        return (true);
    }
}


/*
 * match pattern to str. then copy the match part and return the fresh
 * new char* created.  * flags are REG_ICASE | REG_EXTENDED | REG_NEWLINE
 * (see regex(3)).
 *
 * return value has to be freed.
 */
char *
get_match(const char *__restrict__ str, const char *__restrict__ pattern)
{
    regmatch_t *regmatch;
    size_t match_size;
    char *match;

    assert(str != NULL);

    regmatch = first_match(str, pattern, REG_ICASE | REG_EXTENDED | REG_NEWLINE);
    if (regmatch == NULL)
        return ((char *)NULL);

    match_size = regmatch->rm_eo - regmatch->rm_so;
    match = xcalloc(match_size + 1, sizeof(char));
    
    memcpy(match, str + regmatch->rm_so, match_size);

    free((void *)regmatch);
    return (match);
}


/*
 * replace the part defined by match in str by replace. for example
 * sub_match("foo bar", <regmatch_t that match the bar parT>, "oni") will return
 * "foo oni". sub_match doesn't modify it's arguments, it return a new string.
 *
 * return value has to be freed.
 */
char *
sub_match(const char *str, const regmatch_t *__restrict__ match, const char *replace)
{
    size_t final_size, replace_size, end_size;
    char *result, *cursor;

    assert(str      != NULL);
    assert(replace  != NULL);
    assert(match    != NULL);
    assert(match->rm_so >= 0);
    assert(match->rm_eo > match->rm_so);

    replace_size = strlen(replace);
    end_size = strlen(str) - match->rm_eo;
    final_size = match->rm_so + replace_size + end_size + 1;
    result = xcalloc(final_size, sizeof(char));
    cursor = result;

    memcpy(cursor, str, (unsigned int) match->rm_so);
    cursor += match->rm_so;
    memcpy(cursor, replace, replace_size);
    cursor += replace_size;
    memcpy(cursor, str + match->rm_eo, end_size);

    return(result);
}


/*
 * same as sub_match but change the string reference given by str. *str will
 * be freed so it has to be malloc'd previously.
 */
void inplacesub_match(char **str, regmatch_t *__restrict__ match, const char *replace)
{
    char *old_str;
    assert(str      != NULL);
    assert(*str     != NULL);
    assert(replace  != NULL);
    assert(match    != NULL);

    old_str = *str;
    *str = sub_match(*str, match, replace);

    free((void *)old_str);
    free((void *)match);
}


/*
 * return a char* that contains all tag infos.
 *
 * return value has to be freed.
 */
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


/*
 * call "$EDITOR path" (the environment variable $EDITOR must be set).
 */
bool
user_edit(const char *__restrict__ path)
{

    int error;
    size_t editcmd_size;
    char *editor, *editcmd;

    assert(path != NULL);

    editcmd_size = 1;
    editcmd = xcalloc(editcmd_size, sizeof(char));
    editor = getenv("EDITOR");
    if (editor == NULL)
        die(("please, set the $EDITOR environment variable.\n"));

    concat(&editcmd, &editcmd_size, editor);
    concat(&editcmd, &editcmd_size, " ");
    concat(&editcmd, &editcmd_size, path);

    /* test if the shell is avaiable */
    if ((system(NULL) == 0))
            die(("can't access shell :(\n"));

    error = system(editcmd);

    free((void *)editcmd);
    return (error == 0 ? true : false);
}


/*
 * read fp and tag. the format of the text should bethe same as tagutil_print.
 */
void
update_tag(TagLib_Tag *__restrict__ tag, FILE *__restrict__ fp)
{
    size_t size;
    char *line, *key, *val;

    size = BUFSIZ;
    line = xcalloc(size, sizeof(char));

    while (xgetline(&line, &size, fp)) {
        if (!EMPTY_LINE(line)) {
            if (has_match(line, "^(title|album|artist|year|track|comment|genre)\\s*- \".*\"$")) {
                key = get_match(line, "^(title|album|artist|year|track|comment|genre)");
                val = get_match(line, "\".*\"$");
                val[strlen(val) - 1] = '\0';
                val++; /* glup. we jump over the first char " that's why we free(val - 1) below. */

/* yet another sexy macro. */
#define _SETTER_CALL_IF_MATCH(what, new_value)          \
    do {                                                \
        if (strcmp(key, #what) == 0) {                  \
            (taglib_tag_set_ ## what)(tag, new_value);  \
            goto cleanup;                               \
        }                                               \
    } while (0)

                _SETTER_CALL_IF_MATCH(title,     val);
                _SETTER_CALL_IF_MATCH(album,     val);
                _SETTER_CALL_IF_MATCH(artist,    val);
                _SETTER_CALL_IF_MATCH(comment,   val);
                _SETTER_CALL_IF_MATCH(genre,     val);
                _SETTER_CALL_IF_MATCH(year,      atoi(val));
                _SETTER_CALL_IF_MATCH(track,     atoi(val));

                debug("parser gurk: %s\n", line);
cleanup:
                free((void *)(val - 1));
                free((void *)key);
            }
            else
                debug("unknow line: %s\n", line);
        }
    }
    free((void *)line);
}


/*
 * replace each tagutil keywords by their value. see k* keywords.
 */
char *
eval_tag(const char *__restrict__ pattern, const TagLib_Tag *__restrict__ tag)
{
    regmatch_t *matcher;
    size_t size;
    char *result;
    char buf[5]; /* used to convert track/year tag into string */

    assert(pattern != NULL);
    assert(tag != NULL);

/*
 * replace in result the first argument pattern by the result of the
 * function taglib_tag_(x) (x given in arg).
 */
#define _REPLACE(pattern, x)                                                    \
    do {                                                                        \
        matcher = first_match(result, (pattern), REG_EXTENDED);                 \
        if (matcher != NULL)                                                    \
            inplacesub_match(&result, matcher, (taglib_tag_ ## x)(tag));        \
    } while (0)
/* 
 * _REPLACEI used for track/years because taglib methods return unsigned int, so we
 * "convert" into string (in buf).
 */
#define _REPLACEI(pattern, x)                                                   \
    do {                                                                        \
        matcher = first_match(result, (pattern), REG_EXTENDED);                 \
        if (matcher != NULL) {                                                  \
            (void) snprintf(buf, len(buf), "%02u", (taglib_tag_ ## x)(tag));    \
            inplacesub_match(&result, matcher, buf);                            \
        }                                                                       \
    } while (0)

    /* init result with pattern */
    size = strlen(pattern) + 1;
    result = xcalloc(size, sizeof(char));
    memcpy(result, pattern, size);

    _REPLACE (kTITLE,   title);
    _REPLACE (kALBUM,   album);
    _REPLACE (kARTIST,  artist);
    _REPLACEI(kYEAR,    year);
    _REPLACEI(kTRACK,   track);
    _REPLACE (kCOMMENT, comment);
    _REPLACE (kGENRE,   genre);

    return (result);
}


/*
 * parse argv to find the tagutil_f to use and apply_arg is a pointer to its argument.
 * first_fname is updated to the index of the first file name in argv.
 */
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
    } while (0)

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


/*
 * print the given file's tag to stdin.
 */
bool
tagutil_print(const char *__restrict__ path, TagLib_File *__restrict__ f, const char *__restrict__ arg)
{
    char *infos;
    
    assert(path != NULL);
    assert(f != NULL);

    infos = printable_tag(taglib_file_tag(f));
    (void) printf("FILE: \"%s\"\n", path);
    (void) printf("%s\n\n", infos);

    free((void *)infos);
    return (true);
}


/*
 * print the given file's tag and prompt to ask if tag edit is needed. if
 * answer is yes create a tempfile, fill is with infos, run $EDITOR, then
 * parse the tempfile and update the file's tag.
 */
bool
tagutil_edit(const char *__restrict__ path, TagLib_File *__restrict__ f, const char *__restrict__ arg)
{
    char *tmpfile, *infos;
    FILE *fp;

    assert(path != NULL);
    assert(f != NULL);

    infos = printable_tag(taglib_file_tag(f));
    (void) printf("FILE: \"%s\"\n", path);
    (void) printf("%s\n\n", infos);

    if (yesno("edit this file")) {
        tmpfile = create_tmpfile();

        fp = xfopen(tmpfile, "w");
        (void) fprintf(fp, "%s\n", infos);
        xfclose(fp);

        if (!user_edit(tmpfile)) {
            free((void *)infos);
            remove(tmpfile);
            return (false);
        }

        fp = xfopen(tmpfile, "r");
        update_tag(taglib_file_tag(f), fp);

        if (!taglib_file_save(f))
            die(("can't save file %s.\n", path));

        xfclose(fp);
        remove(tmpfile);
        free((void *)tmpfile);
    }

    free((void *)infos);
    return (true);
}


/*
 * rename the file at path with the given pattern arg. the pattern can use
 * some keywords for tags (see usage()).
 */
bool
tagutil_rename(const char *__restrict__ path, TagLib_File *__restrict__ f, const char *__restrict__ arg)
{    
    char *ftype;
    TagLib_Tag *tag;
    char *result, *dirname, *new_fname;
    size_t result_size, new_fname_size;

    assert(path != NULL);
    assert(f    != NULL);
    assert(arg  != NULL);
    
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
    dirname = xdirname(path);
    result = xcalloc(result_size, sizeof(char));
    if (strcmp(dirname, ".") != 0) {
        concat(&result, &result_size, dirname);
        concat(&result, &result_size, "/");
    }
    concat(&result, &result_size, new_fname);

    /* ask user for confirmation and rename if user want to */
    if (strcmp(path, result) != 0) {
        (void) printf("rename \"%s\" to \"%s\" ", path, result);
        if (yesno(NULL))
            safe_rename(path, result);
    }

    free((void *)dirname);
    free((void *)new_fname);
    free((void *)result);
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
        assert(path != NULL);                                       \
        assert(f    != NULL);                                       \
        assert(arg  != NULL);                                       \
        (taglib_tag_set_##what)(taglib_file_tag(f), hook (arg));    \
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

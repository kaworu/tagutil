/*
 * t_yaml.c
 *
 * yaml tagutil parser and converter.
 */
#include "t_config.h"

#include <string.h>
#include <stdlib.h>

#include "t_yaml.h"
#include "t_toolkit.h"
#include "t_lexer.h"


/*
 * escape " to \" and \ to \\
 *
 * returned value has to be freed.
 */
__t__nonnull(1)
char * yaml_escape(const char *restrict s);


char *
yaml_escape(const char *restrict s)
{
    char *ret;
    int toesc, x;
    unsigned int i;
    size_t slen;

    assert_not_null(s);

    toesc = 0;
    slen = strlen(s);
    for (i = 0; i < slen; i++) {
        if (s[i] == '"' || s[i] == '\\' || s[i] == '\n') {
            toesc++;
            i++;
        }
    }

    if (toesc == 0)
        return (xstrdup(s));

    ret = xcalloc(slen + toesc, sizeof(char));
    x = 0;

    /* take the trailing \0 */
    for (i = 0; i < slen + 1; i++) {
        if (s[i] == '\n') {
            ret[x++] = '\\';
            ret[x++] = 'n';
        }
        else {
            if (s[i] == '"' || s[i] == '\\')
                ret[x++] = '\\';
            ret[x++] = s[i];
        }
    }

    return (ret);
}


char *
tags_to_yaml(const struct tfile *restrict file)
{
    char *ret;
    char *t, *a, *A, *c, *g;
    unsigned int y, T;

    assert_not_null(file);

    t = yaml_escape(file->title(file));
    a = yaml_escape(file->album(file));
    A = yaml_escape(file->artist(file));
    c = yaml_escape(file->comment(file));
    g = yaml_escape(file->genre(file));
    T = file->track(file);
    y = file->year(file);

    xasprintf(&ret,
        "# %s\n"
        "---\n"
        "title:   \"%s\"\n"
        "album:   \"%s\"\n"
        "artist:  \"%s\"\n"
        "year:    %u\n"
        "track:   %u\n"
        "comment: \"%s\"\n"
        "genre:   \"%s\"\n",
            file->path, t, a, A, y, T, c, g);

    free(t); free(a); free(A); free(c); free(g);
    return (ret);
}


/*
 * dummy yaml parser. Only handle our yaml output.
 */
bool
yaml_to_tags(struct tfile *restrict file, FILE *restrict stream)
{
    bool set_somethin; /* true if we have set at least 1 field */
    bool is_intval;
    int (*isetter)(struct tfile *, unsigned int);
    int (*ssetter)(struct tfile *, const char *);
    char c;
    int line;
    char keyword[9]; /* longest is: comment */

    char *value;
    unsigned int ivalue;
    size_t valuelen, i, valueidx;

    assert_not_null(file);
    assert_not_null(stream);

    if (ftrylockfile(stream) != 0)
        errx(-1, "yaml_to_tags: can't lock file descriptor.");

    set_somethin = false;
    line = 1;
    valuelen = BUFSIZ;
    value = xcalloc(valuelen, sizeof(char));

    /* eat first line: ^# <filename>$ */;
    while (!feof_unlocked(stream) && getc_unlocked(stream) != '\n')
        ;

    if (feof_unlocked(stream)) {
        warnx("yaml_to_tags at line %d: EOF reached before header.", line);
        goto free_ret_false;
    }

    line += 1;

    /* eat header: ^---$ */
    if (getc_unlocked(stream) != '-' || getc_unlocked(stream) != '-' ||
            getc_unlocked(stream) != '-' || getc_unlocked(stream) != '\n') {
        warnx("yaml_to_tags at line %d: bad yaml header", line);
        goto free_ret_false;
    }

    c = getc_unlocked(stream);

    for (;;) {
        /* ^keyword:  "value"$ */
        line += 1;

        if (feof_unlocked(stream)) {
            if (!set_somethin)
                warnx("yaml_to_tags at line %d: didn't set any tags.", line);
            break;
        }

        if (!is_letter(c)) {
            warnx("yaml_to_tags at line %d: need a letter to begin, got '%c'", line, c);
            goto free_ret_false;
        }

        /* get the keyword part */
        keyword[0] = '\0';
        for (i = 0; i < len(keyword) - 2 && is_letter(c); i++) {
            keyword[i] = c;
            c = getc_unlocked(stream);
        }
        keyword[i] = '\0';

        /* get the keyword's setter method */
        is_intval = false;
        if (strcmp("genre", keyword) == 0)
            ssetter = file->set_genre;
        else if (strcmp("comment", keyword) == 0)
            ssetter = file->set_comment;
        else if (strcmp("artist", keyword) == 0)
            ssetter = file->set_artist;
        else if (strcmp("album", keyword) == 0)
            ssetter = file->set_album;
        else if (strcmp("title", keyword) == 0)
            ssetter = file->set_title;
        else if (strcmp("track", keyword) == 0) {
            is_intval = true;
            isetter = file->set_track;
        }
        else if (strcmp("year", keyword) == 0) {
            is_intval = true;
            isetter = file->set_year;
        }
        else {
            warnx("yaml_to_tags at line %d: unknown keyword \"%s\"", line, keyword);
            goto free_ret_false;
        }

        /* walk */
        if (c != ':') {
            warnx("yaml_to_tags at line %d: expected ':' after \"%s\" but got '%c'",
                    line, keyword, c);
            goto free_ret_false;
        }

        c = getc_unlocked(stream);
        while (is_blank(c))
            c = getc_unlocked(stream);

        if (is_intval) {
        /* parse Int */
            ivalue = 0;
            while (is_digit(c)) {
                ivalue = 10 * ivalue + (c - '0');
                c = getc_unlocked(stream);
            }

            if (c != '\n') {
                warnx("yaml_to_tags at line %d: expected EOL after Int, got '%c'",
                        line, c);
                goto free_ret_false;
            }
            line += 1;
            (*isetter)(file, ivalue);
        }
        else {
        /* parse String */
            if (c != '"') {
                warnx("yaml_to_tags at line %d: expected '\"' but got '%c'", line, c);
                goto free_ret_false;
            }

            /* read the value */
            valueidx = 0;
            for (;;) {
                /* realloc buffer if needed */
                if (valueidx > valuelen - 2) {
                    valuelen += BUFSIZ;
                    xrealloc(&value, valuelen);
                }

                c = getc_unlocked(stream);

                if (feof_unlocked(stream)) {
                    warnx("yaml_to_tags at line %d: EOF while reading String", line);
                    goto free_ret_false;
                }

                /* handle escape char */
                if (c == '\\') {
                    c = getc_unlocked(stream);
                    if (feof_unlocked(stream)) {
                        warnx("yaml_to_tags at line %d: EOF while reading String", line);
                        goto free_ret_false;
                    }
                    /* handle \n */
                    if (c == 'n')
                        value[valueidx++] = '\n';
                    else
                        value[valueidx++] = c;
                }
                else if (c == '"') {
                    if ((c = getc_unlocked(stream)) != '\n') {
                        warnx("yaml_to_tags at line %d: expected EOL after String, got '%c'",
                                line, c);
                        goto free_ret_false;
                    }
                    line += 1;
                    break;
                }
                else {
                    value[valueidx++] = c;
                    if (c == '\n')
                        line += 1;
                }
            }
            value[valueidx] = '\0';

            /* set the value */
            (*ssetter)(file, value);
        }


        c = getc_unlocked(stream);
        set_somethin = true;
    }

cleanup:
    funlockfile(stream);
    free(value);
    return (set_somethin);

free_ret_false:
    set_somethin = false;
    goto cleanup;
}


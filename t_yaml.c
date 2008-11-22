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
tags_to_yaml(const char *restrict path, const TagLib_Tag *restrict tags)
{
    char *ret, *die;
    size_t retlen;
    /* handle 2008\0 at max, so it's ok for the next 7'991 years */
    char buf[5];

    assert_not_null(path);
    assert_not_null(tags);

    retlen = 1;
    ret = xcalloc(retlen, sizeof(char));

    concat(&ret, &retlen, "# ");
    concat(&ret, &retlen, path);
    concat(&ret, &retlen, "\n---\ntitle:   \"");
    concat(&ret, &retlen, die = yaml_escape(taglib_tag_title(tags)));
    free(die);
    concat(&ret, &retlen, "\"\nalbum:   \"");
    concat(&ret, &retlen, die = yaml_escape(taglib_tag_album(tags)));
    free(die);
    concat(&ret, &retlen, "\"\nartist:  \"");
    concat(&ret, &retlen, die = yaml_escape(taglib_tag_artist(tags)));
    free(die);
    concat(&ret, &retlen, "\"\nyear:    ");
    (void)snprintf(buf, len(buf), "%u", taglib_tag_year(tags));
    concat(&ret, &retlen, buf);
    concat(&ret, &retlen, "\ntrack:   ");
    (void)snprintf(buf, len(buf), "%u", taglib_tag_track(tags));
    concat(&ret, &retlen, buf);
    concat(&ret, &retlen, "\ncomment: \"");
    concat(&ret, &retlen, die = yaml_escape(taglib_tag_comment(tags)));
    free(die);
    concat(&ret, &retlen, "\"\ngenre:   \"");
    concat(&ret, &retlen, die = yaml_escape(taglib_tag_genre(tags)));
    free(die);
    concat(&ret, &retlen, "\"\n");


    return (ret);
}


/*
 * dummy yaml parser. Only handle our yaml output.
 */
bool
yaml_to_tags(TagLib_Tag *restrict tags, FILE *restrict stream)
{
    bool set_somethin; /* true if we have set at least 1 field */
    bool is_intval;
    void *setter;
    char c;
    int line;
    char keyword[9]; /* longest is: comment */

    char *value;
    unsigned int ivalue;
    size_t valuelen, i, valueidx;

    assert_not_null(tags);
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
            setter = taglib_tag_set_genre;
        else if (strcmp("comment", keyword) == 0)
            setter = taglib_tag_set_comment;
        else if (strcmp("artist", keyword) == 0)
            setter = taglib_tag_set_artist;
        else if (strcmp("album", keyword) == 0)
            setter = taglib_tag_set_album;
        else if (strcmp("title", keyword) == 0)
            setter = taglib_tag_set_title;
        else if (strcmp("track", keyword) == 0) {
            is_intval = true;
            setter = taglib_tag_set_track;
        }
        else if (strcmp("year", keyword) == 0) {
            is_intval = true;
            setter = taglib_tag_set_year;
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
            (*(void (*)(TagLib_Tag *, unsigned int))setter)(tags, ivalue);
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
            (*(void (*)(TagLib_Tag *, const char *))setter)(tags, value);
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


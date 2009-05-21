/*
 * t_yaml.c
 *
 * yaml tagutil parser and converter.
 */
#include "t_config.h"

#include <string.h>
#include <stdlib.h>

#include "t_toolkit.h"
#include "t_lexer.h"
#include "t_file.h"
#include "t_yaml.h"


/*
 * escape " to \" and \ to \\
 *
 * returned value has to be freed.
 */
_t__nonnull(1)
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
    slen += 1;
    for (i = 0; i < slen; i++) {
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
    char **tagkeys;
    char *val, *yaml, *ret, *oldret, *endptr;
    int i, count;

    assert_not_null(file);

    tagkeys = file->tagkeys(file);
    if (tagkeys == NULL)
        errx(-1, "NULL tagkeys (%s backend)", file->lib);

    xasprintf(&ret, "# %s\n---\n", file->path);

    count = file->tagcount(file);
    for (i = 0; i < count; i++) {
        oldret = ret;
        val = file->get(file, tagkeys[i]);
        if (val == NULL)
            errx(-1, "bad tagkeys/get (%s backend)", file->lib);
        yaml = yaml_escape(val);
        (void)strtoul(yaml, &endptr, 10);
        if (endptr == yaml || *endptr != '\0')
        /* looks like string */
            (void)xasprintf(&ret, "%s%s: \"%s\"\n", oldret, tagkeys[i], yaml);
        else
        /* looks like int */
            (void)xasprintf(&ret, "%s%s: %s\n", oldret, tagkeys[i], yaml);

        xfree(tagkeys[i]);
        xfree(val);
        xfree(oldret);
        xfree(yaml);
    }
    xfree(tagkeys);

    return (ret);
}


/*
 * dummy yaml parser. Only handle our yaml output.
 */
bool
yaml_to_tags(struct tfile *restrict file, FILE *restrict stream)
{
    bool set_somethin; /* true if we have set at least 1 field */
    char c;
    int line;
    char keyword[9]; /* longest is: comment */

    char *value;
    unsigned int ivalue;
    size_t valuelen, i, valueidx;

    assert_not_null(file);
    assert_not_null(stream);

    valuelen = BUFSIZ;
    value = xcalloc(valuelen, sizeof(char));

    if (ftrylockfile(stream) != 0)
        errx(-1, "yaml_to_tags: can't lock file descriptor.");

    set_somethin = false;
    line = 1;

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

        /* walk */
        if (c != ':') {
            warnx("yaml_to_tags at line %d: expected ':' after \"%s\" but got '%c'",
                    line, keyword, c);
            goto free_ret_false;
        }

        c = getc_unlocked(stream);
        while (is_blank(c))
            c = getc_unlocked(stream);

        if (c != '"') {
        /* parse Int */
            if (!is_digit(c)) {
                warnx("yaml_to_tags at line %d: expected String or Integer, got '%c'",
                        line, c);
                goto free_ret_false;
            }
            ivalue = 0;
            while (is_digit(c)) {
                ivalue = 10 * ivalue + (c - '0'); /* TODO: check overflow? */
                c = getc_unlocked(stream);
            }
            if (c != '\n') {
                warnx("yaml_to_tags at line %d: expected EOL after Int, got '%c'",
                        line, c);
                goto free_ret_false;
            }
            line += 1;
            while ((unsigned int)snprintf(value, valuelen, "%u", ivalue) > valuelen - 1) {
                valuelen += BUFSIZ;
                xrealloc(&value, valuelen);
            }
        }
        else {
        /* parse String */
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
                    break; /* get out of for (;;) */
                }
                else {
                    value[valueidx++] = c;
                    if (c == '\n')
                        line += 1;
                }
            }
            value[valueidx] = '\0';
        }

        file->set(file, keyword, value);
        c = getc_unlocked(stream);
        set_somethin = true;
    }

cleanup:
    funlockfile(stream);
    xfree(value);
    return (set_somethin);

free_ret_false:
    set_somethin = false;
    goto cleanup;
}


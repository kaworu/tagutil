/*
 * t_yaml.c
 *
 * YAML tagutil parser and converter.
 */
#include "t_config.h"

#include "t_yaml.h"
#include "t_toolkit.h"
#include "t_lexer.h"
#include "tagutil.h" /* FIXME */


char *
tags_to_yaml(const char *restrict path, const TagLib_Tag *restrict tags)
{
    char *ret, *template, *die;
    size_t retlen;

    assert_not_null(path);
    assert_not_null(tags);

    template =  "\n"
                "---\n"
                "title:   \"%t\""
                "album:   \"%a\""
                "artist:  \"%A\""
                "year:    \"%y\""
                "track:   \"%T\""
                "comment: \"%c\""
                "genre:   \"%g\"";

    retlen = 1;
    ret = xcalloc(retlen, sizeof(char));


    concat(&ret, &retlen, "# ");
    concat(&ret, &retlen, path);
    concat(&ret, &retlen, die = eval_tag(template, tags));
    free(die);

    return (ret);
}


/*
 * XXX: could use getc_unlocked(3) w/ flockfile(3)?
 *
 * dummy yaml parser. Only handle our YAML output.
 */
bool
yaml_to_tags(const TagLib_Tag *restrict tags, FILE *restrict stream)
{
    bool set_somethin; /* true if we have set at least 1 field */
    bool is_intval;
    void *setter;
    char c;
    int line;
    char keyword[8]; /* longest is: comment */

    char *value;
    size_t valuelen, i, valueidx;

    set_somethin = false;
    line = 1;
    valuelen = BUFSIZ;
    value = xcalloc(valuelen, sizeof(char));

    /* eat first line: ^# <filename>$ */;
    while (!feof(stream) && getc(stream) != '\n')
    if (feof(stream)) {
        warnx("yaml_to_tags at line %d: EOF reached before header.", line);
        goto free_ret_false;
    }

    line += 1;

    /* eat header: ^---$ */
    if (getc(stream) != '-' || getc(stream) != '-' ||
            getc(stream) != '-' || getc(stream) != '\n') {
        warnx("yaml_to_tags at line %d: EOF reached before data.", line);
        goto free_ret_false;
    }

    for (;;) {
        /* ^keyword:  "value"$ */
        line += 1;

        if (feof(stream)) {
            if (!set_somethin)
                warnx("yaml_to_tags at line %d: didn't set any tags.", line);
            break;
        }

        if (!is_letter(c)) {
            warnx("yaml_to_tags at line %d: need a letter to begin.", line);
            goto free_ret_false;
        }

        /* get the keyword part */
        keyword[0] = '\0';
        for (i = 0; i < len(keyword) - 2 && is_letter(c); i++) {
            keyword[i] = c;
            c = getc(stream);
        }
        keyword[i + 1] = '\0';

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

        while (!feof(stream) && is_blank(c = getc(stream)))
            /* eat blank chars */;

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

            c = getc(stream);

            if (feof(stream)) {
                warnx("yaml_to_tags at line %d: EOF while reading String", line);
                goto free_ret_false;
            }

            /* handle escape char */
            if (c == '\\') {
                c = getc(stream);
                if (feof(stream)) {
                    warnx("yaml_to_tags at line %d: EOF while reading String", line);
                    goto free_ret_false;
                }
                value[valueidx++] = c;
            }
            else if (c == '"')
                break;
            else
                value[valueidx++] = c;
        }
        value[valueidx] = '\0';

        /* set the value */
        if (is_intval)
            ((void (*)(TagLib_Tag *, unsigned int))setter)(tags, atoi(value));
        else
            ((void (*)(TagLib_Tag *, const char *))setter)(tags, value);
        set_somethin = true;
    }

    free(value);
    return (set_somethin);
free_ret_false:
    free(value);
    return (false);
}

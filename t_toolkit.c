/*
 * t_toolkit.c
 *
 * handy functions toolkit for tagutil.
 */

#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#include "config.h"
#include "t_toolkit.h"


bool
xgetline(char **line, size_t *size, FILE *restrict fp)
{
    char *cursor;
    size_t end;

    assert_not_null(fp);
    assert_not_null(size);
    assert_not_null(line);

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
        (void)fgets(cursor, BUFSIZ, fp);
        end += strlen(cursor);

        if (feof(fp) || (*line)[end - 1] == '\n') { /* end of file or end of line */
        /* chomp trailing \n if any */
            if ((*line)[0] != '\0' && (*line)[end - 1] == '\n')
                (*line)[end - 1] = '\0';
            return (end);
        }
    }
}


regmatch_t *
first_match(const char *restrict str, const char *restrict pattern, const int flags)
{
    regex_t regex;
    regmatch_t *regmatch;
    int error;
    char *errbuf;

    assert_not_null(str);
    assert_not_null(pattern);

    regmatch = xmalloc(sizeof(regmatch_t));
    error = regcomp(&regex, pattern, flags);

    if (error != 0)
        goto error_label;

    error = regexec(&regex, str, 1, regmatch, 0);
    regfree(&regex);

    switch (error) {
    case 0:
        return (regmatch);
    case REG_NOMATCH:
        free(regmatch);
        return (NULL);
    default:
error_label:
        errbuf = xcalloc(BUFSIZ, sizeof(char));
        (void)regerror(error, &regex, errbuf, BUFSIZ);
        errx(-1, "%s", errbuf);
        /* NOTREACHED */
    }
}


bool
has_match(const char *restrict str, const char *restrict pattern)
{
    regmatch_t *match;

    assert_not_null(pattern);

    if (str == NULL)
        return (false);

    match = first_match(str, pattern, REG_ICASE | REG_EXTENDED | REG_NEWLINE | REG_NOSUB);

    if (match == NULL)
        return (false);
    else {
        free(match);
        return (true);
    }
}


char *
get_match(const char *restrict str, const char *restrict pattern)
{
    regmatch_t *regmatch;
    size_t match_size;
    char *match;

    assert_not_null(str);

    regmatch = first_match(str, pattern, REG_ICASE | REG_EXTENDED | REG_NEWLINE);
    if (regmatch == NULL)
        return (NULL);

    match_size = regmatch->rm_eo - regmatch->rm_so;
    match = xcalloc(match_size + 1, sizeof(char));

    memcpy(match, str + regmatch->rm_so, match_size);

    free(regmatch);
    return (match);
}


char *
sub_match(const char *str, const regmatch_t *restrict match, const char *replace)
{
    size_t final_size, replace_size, end_size;
    char *result, *cursor;

    assert_not_null(str);
    assert_not_null(replace);
    assert_not_null(match);
    assert(match->rm_so >= 0);
    assert(match->rm_eo > match->rm_so);

    replace_size = strlen(replace);
    end_size = strlen(str) - match->rm_eo;
    final_size = match->rm_so + replace_size + end_size + 1;
    result = xcalloc(final_size, sizeof(char));
    cursor = result;

    memcpy(cursor, str, (unsigned int)match->rm_so);
    cursor += match->rm_so;
    memcpy(cursor, replace, replace_size);
    cursor += replace_size;
    memcpy(cursor, str + match->rm_eo, end_size);

    return(result);
}


void inplacesub_match(char **str, regmatch_t *restrict match, const char *replace)
{
    char *old_str;

    assert_not_null(str);
    assert_not_null(*str);
    assert_not_null(replace);
    assert_not_null(match);

    old_str = *str;
    *str = sub_match(*str, match, replace);

    free(old_str);
}


bool
yesno(const char *restrict question)
{
    char buffer[5]; /* strlen("yes\n\0") == 5 */

    for (;;) {
        if (feof(stdin))
            return (false);

        (void)memset(buffer, '\0', len(buffer));

        if (question != NULL)
            (void)printf("%s", question);
        (void)printf("? [y/n] ");

        (void)fgets(buffer, len(buffer), stdin);

        /* if any, eat stdin characters that didn't fit into buffer */
        if (buffer[strlen(buffer) - 1] != '\n') {
            while (getc(stdin) != '\n' && !feof(stdin))
                ;
        }

        if (has_match(buffer, "^(n|no)$"))
            return (false);
        else if (has_match(buffer, "^(y|yes)$"))
            return (true);
    }
}

#if 0
int
read_int(const int min, const int max)
{
    char buffer[5]; /* strlen("yes\n\0") == 5 */

    for (;;) {
        if (feof(stdin))
            return (false);

        (void)memset(buffer, '\0', len(buffer));

        if (question != NULL)
            (void)printf("%s", question);
        (void)printf("? [y/n] ");

        (void)fgets(buffer, len(buffer), stdin);

        /* if any, eat stdin characters that didn't fit into buffer */
        if (buffer[strlen(buffer) - 1] != '\n') {
            while (getc(stdin) != '\n' && !feof(stdin))
                ;
        }

        if (has_match(buffer, "^(n|no)$"))
            return (false);
        else if (has_match(buffer, "^(y|yes)$"))
            return (true);
    }
}

#endif

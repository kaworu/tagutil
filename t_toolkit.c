/*
 * t_toolkit.c
 *
 * handy functions toolkit for tagutil.
 */


#include "t_toolkit.h"
#include <stdarg.h>
#include <assert.h>
#include <errno.h>


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
        goto error_label;

    error = regexec(&regex, str, 1, regmatch, 0);
    regfree(&regex);

    switch (error) {
    case 0:
        return (regmatch);
    case REG_NOMATCH:
        free(regmatch);
        return ((regmatch_t *)NULL);
    default:
error_label:
        errbuf = xcalloc(BUFSIZ, sizeof(char));
        (void) regerror(error, &regex, errbuf, BUFSIZ);
        die(("%s", errbuf));
        /* NOTREACHED */
    }
}


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
}


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

    exit(errno == 0 ? -1: errno);
}


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


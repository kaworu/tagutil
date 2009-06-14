/*
 * t_toolkit.c
 *
 * handy functions toolkit for tagutil.
 *
 */
#include "t_config.h"

#include "t_toolkit.h"


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
        freex(regmatch);
        return (NULL);
    default:
error_label:
        errbuf = xcalloc(BUFSIZ, sizeof(char));
        (void)regerror(error, &regex, errbuf, BUFSIZ);
        errx(-1, "first_match: %s", errbuf);
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
        freex(match);
        return (true);
    }
}


bool
yesno(const char *restrict question)
{
    char *endl;
    char buffer[5]; /* strlen("yes\n\0") == 5 */
    extern bool Yflag, Nflag;

    for (;;) {
        if (feof(stdin) && !Yflag && !Nflag)
            return (false);

        (void)memset(buffer, '\0', len(buffer));

        if (question != NULL) {
            (void)printf("%s? [y/n] ", question);
            (void)fflush(stdout);
        }

        if (Yflag) {
            (void)printf("y\n");
            return (true);
        }
        else if (Nflag) {
            (void)printf("n\n");
            return (false);
        }

        if (fgets(buffer, len(buffer), stdin) == NULL) {
            if (feof(stdin))
                return (false);
            else
                err(errno, "fgets");
        }

        endl = strchr(buffer, '\n');
        if (endl == NULL) {
        /* buffer didn't receive EOL, must still be on stdin */
            while (getc(stdin) != '\n' && !feof(stdin))
                continue;
        }
        else {
            *endl = '\0';
            strtolower(buffer);
            if (strcmp(buffer, "n") == 0 || strcmp(buffer, "no") == 0)
                return (false);
            else if (strcmp(buffer, "y") == 0 || strcmp(buffer, "yes") == 0)
                return (true);
        }
    }
}


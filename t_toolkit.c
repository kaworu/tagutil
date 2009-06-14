/*
 * t_toolkit.c
 *
 * handy functions toolkit for tagutil.
 *
 */
#include "t_config.h"

#include "t_toolkit.h"


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


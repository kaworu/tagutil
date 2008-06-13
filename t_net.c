/*
 * t_net.c
 *
 * HTTP request function for tagutil.
 * loosely cp from client example of getaddrinfo(3) man page.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "t_toolkit.h"
#include "t_net.h"

static int
mb_opensock(void)
{
    int sfd; /* socket file descriptor */
    int ret;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;        /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;    /* TCP socket */

    ret = getaddrinfo(MB_URI, MB_PORT, &hints, &result);
    if (ret != 0)
        die(("getaddrinfo: %s\n", gai_strerror(ret)));

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */
        close(sfd);
    }

    if (rp == NULL) /* No address succeeded */
        die(("Could not connect\n"));

    freeaddrinfo(result); /* No longer needed */

    return (sfd);
}


char *
mb_get(const char *__restrict__ arg)
{
    int sfd; /* socket file descriptor */
    char *getmsg; /* GET message to send. */
    size_t getmsg_size;
    char *buf; /* buffer to store the server's response */
    size_t buf_size, end;
    char *cursor;
    ssize_t nread; /* number of given by read(2) */

    sfd = mb_opensock();

    getmsg_size = 1;
    getmsg = xcalloc(getmsg_size, sizeof(char));
    concat(&getmsg, &getmsg_size, "GET http://"MB_URI"/");
    if (arg != NULL && arg[0] != '\0')
        concat(&getmsg, &getmsg_size, arg);
    concat(&getmsg, &getmsg_size, "\n");

    if (write(sfd, getmsg, getmsg_size) == -1)
        die(("write() failed\n"));

    buf = NULL;
    buf_size = end = 0;

    for (;;) {
        if (buf == NULL || end > 0) {
            buf_size += BUFSIZ;
            buf = xrealloc(buf, buf_size);
        }

        cursor = buf + end;
        nread = read(sfd, cursor, BUFSIZ);
        if (nread == -1)
            die(("read() failed\n"));
        end += nread;

        if (nread == 0 || nread < BUFSIZ)
            break;
    }
    buf[end] = '\0'; /* ensure NULL-terminate string */

    close(sfd);
    free(getmsg);

    return (buf);
}

#if 0
int main(int argc, char *argv[])
{
    char *s, *arg;

    s = NULL;
    arg = (argc < 2 ? NULL : argv[1]);
    s = mb_get(arg);

    printf("%s\n", s);

    free(s);
    return (0);
}
#endif

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

#include "config.h"
#include "t_net.h"
#include "t_toolkit.h"


static int net_opensock(const char *__restrict__ host, const char *__restrict__ port)
    __attribute__ ((__nonnull__ (1, 2)));


static int
net_opensock(const char *__restrict__ host, const char *__restrict__ port)
{
    int sfd; /* socket file descriptor */
    int ret;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;        /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;    /* TCP socket */

    ret = getaddrinfo(host, port, &hints, &result);
    if (ret != 0)
        errx(-1, "getaddrinfo: %s", gai_strerror(ret));

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
        errx(-1, "Could not connect");

    freeaddrinfo(result); /* No longer needed */

    return (sfd);
}


char *
net_get(const char *__restrict__ host, const char *__restrict__ port,
        const char *__restrict__ arg)
{
    int sfd; /* socket file descriptor */
    char *getmsg; /* GET message to send. */
    size_t getmsg_size;
    char *buf; /* buffer to store the server's response */
    size_t buf_size, end;
    char *cursor;
    ssize_t nread; /* number of char given by read(2) */

    assert_not_null(host);
    assert_not_null(port);

    sfd = net_opensock(host, port);

    /* create the get message */
    getmsg_size = 1;
    getmsg = xcalloc(getmsg_size, sizeof(char));
    concat(&getmsg, &getmsg_size, "GET http://");
    concat(&getmsg, &getmsg_size, host);
    concat(&getmsg, &getmsg_size, "/");
    if (arg != NULL && arg[0] != '\0')
        concat(&getmsg, &getmsg_size, arg);
    concat(&getmsg, &getmsg_size, "\n");

    if (write(sfd, getmsg, getmsg_size) == -1)
        err(errno, "write() failed");

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
            err(errno, "read() failed");
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
    if (argc < 2) {
        printf("usage: %s host [args]\n", argv[0]);
        return (0);
    }

    s = net_get(argv[1], "80", (argc > 2 ? argv[2] : NULL));

    printf("%s\n", s);

    free(s);
    return (0);
}
#endif

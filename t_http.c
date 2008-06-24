/*
 * t_http.c
 *
 * HTTP request function for tagutil.
 * loosely cp from client example of getaddrinfo(3) man page.
 */
#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>
#include <unistd.h>

#include "config.h"
#include "t_http.h"
#include "t_toolkit.h"


/*
 * open a socket for given host:port (IPv4 or IPv6 with TCP).
 *
 * FIXME:   maybe it should take a *bool to say if all was OK, but then it also
 *          need a char** to store the message. It would be strong, but messy.
 */
static int http_opensock(const char *restrict host, const char *restrict port)
    __attribute__ ((__nonnull__ (1, 2)));


static int
http_opensock(const char *restrict host, const char *restrict port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd; /* socket file descriptor */
    int ret;

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
http_request(const char *restrict host, const char *restrict port,
        const char *restrict method, const char *restrict arg)
{
    int     sfd;    /* socket file descriptor */
    char   *request;    /* GET|POST|PUT|DELETE command to send. */
    size_t  request_size;
    char   *buf;    /* buffer to store the server's response */
    size_t  buf_size, end;
    char   *cursor; /* cursor in buf */
    ssize_t nread;  /* number of char given by read(2) */

    assert_not_null(host);
    assert_not_null(port);

    sfd = http_opensock(host, port);

    /*
     * create the request message.
     * we need "$method http://$host/$arg\n"
     */
    request_size = 1;
    request = xcalloc(request_size, sizeof(char));

    /* "$method" */
    concat(&request, &request_size, method);
    /* "http:// */
    if (has_match(host, "^http://"))
        concat(&request, &request_size, " ");
    else
        concat(&request, &request_size, " http://");
    /* $host */
    concat(&request, &request_size, host);
    /* /$arg */
    if (arg != NULL && arg[0] != '\0') {
        assert(arg[0] == '/');
        concat(&request, &request_size, arg);
    }
    else
        concat(&request, &request_size, "/");
    /* \n" */
    concat(&request, &request_size, "\n");

    if (write(sfd, request, request_size) == -1)
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

        if (nread < BUFSIZ)
            break;
    }
    buf[end] = '\0'; /* ensure NULL-terminate string */

    close(sfd);
    free(request);

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

    s = http_request(argv[1], "80", HTTP_GET, (argc > 2 ? argv[2] : NULL));

    printf("%s\n", s);

    free(s);
    return (0);
}
#endif

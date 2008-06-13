#ifndef T_NET_H
#define T_NET_H
/*
 * t_net.h
 *
 * HTTP request function for tagutil.
 */
#define MB_URI  "localhost"
#define MB_PORT "80"

/*
 * query the MB server. arg is the argument encoded in a HTML way
 * ?foo=bar&oni=42. It return the server's response.
 *
 * return value has to be freed.
 */
char* mb_get(const char *__restrict__ arg);
#endif /* !T_NET_H */

#ifndef _ARPA_INET_H
#define _ARPA_INET_H

#include <inttypes.h>
#include <net/hton.h>
#include <netinet/in.h>

in_addr_t   inet_addr(const char *);
char        *inet_ntoa(struct in_addr);
const char  *inet_ntop(int, const void *, char *,socklen_t);
int         inet_pton(int, const char *, void *);

#endif

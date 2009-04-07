#ifndef _NETINET_IN_H
#define _NETINET_IN_H

#ifndef SYS_SOCK_CONSTANTS_ONLY
#include <inttypes.h>
#include <sys/socket.h>

typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct in_addr
{
  in_addr_t s_addr;
};


struct sockaddr_in
{
  sa_family_t sin_family;
  in_port_t sin_port;
  struct in_addr sin_addr;
};

#endif

#ifndef IN_PROTOCOLS_DEFINED
#define IPPROTO_IP    0
#define IPPROTO_IPV6  1
#define IPPROTO_ICMP  2
#define IPPROTO_RAW   3
#define IPPROTO_TCP   4
#define IPPROTO_UDP   5
#endif

#define IN_CLASSA_NSHIFT 24

#define INADDR_ANY        0
#define INADDR_BROADCAST  0xffffffff

#define INET_ADDRSTRLEN   16

#endif

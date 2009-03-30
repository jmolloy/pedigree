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
enum Protocols
{
  IPPROTO_IP = 0,
  IPPROTO_IPV6,
  IPPROTO_ICMP,
  IPPROTO_RAW,
  IPPROTO_TCP,
  IPPROTO_UDP
};
#endif

#define INADDR_ANY        0
#define INADDR_BROADCAST  0xffffffff

#define INET_ADDRSTRLEN   16

#endif

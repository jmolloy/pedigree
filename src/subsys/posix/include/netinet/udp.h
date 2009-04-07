#ifndef _NETINET_UDP_H
#define _NETINET_UDP_H

#include <sys/types.h>

struct udphdr
{
  uint16_t uh_sport;
  uint16_t uh_dport;
  uint16_t uh_ulen;
  uint16_t uh_sum;
};

#endif

#ifndef _NET_HTON_H
#define _NET_HTON_H

#ifndef COMPILING_SUBSYS
#include <inttypes.h>
#endif

unsigned int htonl(unsigned int n);
unsigned int ntohl(unsigned int n);

unsigned short htons(unsigned short n);
unsigned short ntohs(unsigned short n);

#endif

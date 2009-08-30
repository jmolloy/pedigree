#ifndef _NET_HTON_H
#define _NET_HTON_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned int htonl(unsigned int n);
unsigned int ntohl(unsigned int n);

unsigned short htons(unsigned short n);
unsigned short ntohs(unsigned short n);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif

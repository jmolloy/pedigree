#ifndef _NET_HTON_H
#define _NET_HTON_H

#include <inttypes.h>

/// \todo Support big-endian properly and in a way that works

#if (LITTLE_ENDIAN) && (BIG_ENDIAN)
  //#error LITTLE_ENDIAN and BIG_ENDIAN are defined at the same time!
#endif

#if !(LITTLE_ENDIAN) && !(BIG_ENDIAN)
  //#error Neither LITTLE_ENDIAN or BIG_ENDIAN are defined!
#endif

#if LITTLE_ENDIAN || 1

#define BS16(x) (((x&0xFF00)>>8)|((x&0x00FF)<<8))
#define BS32(x) (((x&0xFF000000)>>24)|((x&0x00FF0000)>>8)|((x&0x0000FF00)<<8)|((x&0x000000FF)<<24))

#define htonl(x)  (BS32(x))
#define ntohl(x)  (BS32(x))
#define htons(x)  (BS16(x))
#define ntohs(x)  (BS16(x))

#else // BIG_ENDIAN

#define htonl(x)  (x)
#define htons(x)  (x)
#define ntohs(x)  (x)
#define ntohl(x)  (x)

#endif

#endif

#ifndef __MACHINE_ENDIAN_H__
#define __MACHINE_ENDIAN_H__

#include <sys/config.h>

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321

#define LITTLE_ENDIAN __LITTLE_ENDIAN
#define BIG_ENDIAN __BIG_ENDIAN

#ifdef __IEEE_LITTLE_ENDIAN
#define __BYTE_ORDER __LITTLE_ENDIAN
#else
#define __BYTE_ORDER __BIG_ENDIAN
#endif

#define BYTE_ORDER __BYTE_ORDER

#endif /* __MACHINE_ENDIAN_H__ */

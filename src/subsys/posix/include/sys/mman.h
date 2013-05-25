#ifndef _SYS_MMAN_H
#define _SYS_MMAN_H

_BEGIN_STD_C

#define PROT_READ 0
#define PROT_WRITE 1
#define PROT_EXEC 2
#define PROT_NONE 4

#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_FIXED 4
#define MAP_ANON 8

#define MAP_USERSVD 0x10000

#define MAP_FAILED ((void*) 0)

#define MS_ASYNC        0x1
#define MS_SYNC         0x2
#define MS_INVALIDATE   0x4

#include <sys/types.h>

void  *mmap(void *, size_t, int, int, int, off_t);
int    munmap(void *, size_t);
int    mprotect(void *addr, size_t len, int prot);
int    msync(void *addr, size_t len, int flags);

_END_STD_C

#endif

#ifndef SYS_UIO_H
#define SYS_UIO_H

#include <sys/types.h>

struct iovec
{
  void* iov_base;
  size_t iov_len;
};

/// \todo Write these
#define readv(a, b, c) (-1)
#define writev(a, b, c) (-1)


#endif

#ifndef _NET_IF_H
#define _NET_IF_H

#include <sys/types.h>
#include <unistd.h>

#define IF_NAMESIZE 64
#define IFNAMSIZ    IF_NAMESIZE

struct if_nameindex
{
    unsigned if_index;
    char*     if_name;
};

/// \todo ioctl's on network devices to get information
struct ifreq {
    char ifr_name[IFNAMSIZ];
};

_BEGIN_STD_C

unsigned                _EXFUN(if_nametoindex, (const char*));
char*                   _EXFUN(if_indextoname, (unsigned, char*));
struct if_nameindex*    _EXFUN(if_nameindex, (void));
void                    _EXFUN(if_freenameindex, (struct if_nameindex*));

_END_STD_C

#endif

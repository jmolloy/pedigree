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

struct ifconf
{
    int ifc_len;
    union
    {
        caddr_t ifcu_buf;
        struct ifreq *ifcu_req;
    } ifc_ifcu;
};

#define ifc_buf ifc_ifcu.ifcu_buf
#define ifc_req ifc_ifcu.ifcu_req

#define SIOCGSIZIFCONF      1
#define SIOCFGIADDR         2
#define SIOCSIFADDR         3
#define SIOCGIFCONF         4

_BEGIN_STD_C

unsigned                _EXFUN(if_nametoindex, (const char*));
char*                   _EXFUN(if_indextoname, (unsigned, char*));
struct if_nameindex*    _EXFUN(if_nameindex, (void));
void                    _EXFUN(if_freenameindex, (struct if_nameindex*));

_END_STD_C

#endif

/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

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
    struct sockaddr ifr_addr;
    short ifr_flags;
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
#define SIOCGIFFLAGS        5
#define SIOCGIFNETMASK      6
#define SIOCGIFADDR         7

#define IFF_UP              1
#define IFF_LOOPBACK        2

_BEGIN_STD_C

unsigned                _EXFUN(if_nametoindex, (const char*));
char*                   _EXFUN(if_indextoname, (unsigned, char*));
struct if_nameindex*    _EXFUN(if_nameindex, (void));
void                    _EXFUN(if_freenameindex, (struct if_nameindex*));

_END_STD_C

#endif

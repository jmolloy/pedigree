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

#ifndef	_SYS_SOCKET_H
#define	_SYS_SOCKET_H

#include <inttypes.h>
#include <sys/types.h>
#include <sys/uio.h>

#define _SS_MAXSIZE	128

typedef size_t socklen_t;

#ifndef _SA_FAMILY_T
#define _SA_FAMILY_T
typedef uint32_t sa_family_t;
#endif

struct sockaddr
{
  sa_family_t sa_family;
  char sa_data[14];
};

struct sockaddr_storage
{
    sa_family_t ss_family;
    char        ss_data[_SS_MAXSIZE];
} __attribute__((aligned(4)));

struct msghdr
{
  void*         msg_name;
  socklen_t     msg_namelen;
  struct iovec* msg_iov;
  int32_t       msg_iovlen;
  void*         msg_control;
  socklen_t     msg_controllen;
  int32_t       msg_flags;
};

struct cmsghdr
{
  socklen_t     cmsg_len;
  int           cmsg_level;
  int           cmsg_type;
};

/// \todo implement these
#define CMSG_SPACE(a)      0
#define CMSG_DATA(a)       0
#define CMSG_FIRSTHDR(a)   0
#define CMSG_NXTHDR(a, b)  0

struct linger
{
  int l_onoff;
  int l_linger;
};

#define SOCK_DGRAM      1
#define SOCK_RAW        2
#define SOCK_STREAM     3
#define SOCK_SEQPACKET  4
#define SOCK_RDM        5

#define SOL_SOCKET      0

#define SO_ACCEPTCONN   0
#define SO_BROADCAST    1
#define SO_DEBUG        2
#define SO_DONTROUTE    3
#define SO_ERROR        4
#define SO_KEEPALIVE    5
#define SO_LINGER       6
#define SO_OOBINLINE    7
#define SO_RCVBUF       8
#define SO_RCVLOWAT     9
#define SO_RCVTIMEO     10
#define SO_REUSEADDR    11
#define SO_SNDBUF       12
#define SO_SNDLOWAT     13
#define SO_SNDTIMEO     14
#define SO_TYPE         15
#define SO_MAX          16

#define SOMAXCONN     65536

#define MSG_CTRUNC    0
#define MSG_DONTROUTE 1
#define MSG_EOR       2
#define MSG_OOB       4
#define MSG_NOSIGNAL  8
#define MSG_PEEK      16
#define MSG_TRUNC     32
#define MSG_WAITALL   64

#define PF_INET   1
#define PF_INET6  2
#define PF_UNIX   3
#define PF_SOCKET 4
#define PF_MAX    5
#define PF_UNSPEC (PF_MAX+1)

#define AF_INET   (PF_INET)
#define AF_INET6  (PF_INET6)
#define AF_UNIX   (PF_UNIX)
#define AF_UNSPEC (PF_UNSPEC)

#define  SHUT_RD 0 /** Disables receiving */
#define  SHUT_WR 1 /** Disables sending */
#define  SHUT_RDWR 2 /** Disables recv/send */

#define SCM_RIGHTS    1

#ifdef __cplusplus
extern "C" {
#endif

int     accept(int, struct sockaddr *, socklen_t *);
int     bind(int, const struct sockaddr *, socklen_t);
int     connect(int, const struct sockaddr *, socklen_t);
int     getpeername(int, struct sockaddr *, socklen_t *);
int     getsockname(int, struct sockaddr *, socklen_t *);
int     getsockopt(int, int, int, void *, socklen_t *);
int     listen(int, int);
ssize_t recv(int, void *, size_t, int);
ssize_t recvfrom(int, void *, size_t, int, struct sockaddr *, socklen_t *);
ssize_t recvmsg(int, struct msghdr *, int);
ssize_t send(int, const void *, size_t, int);
ssize_t sendmsg(int, const struct msghdr *, int);
ssize_t sendto(int, const void *, size_t, int, const struct sockaddr *,socklen_t);
int     setsockopt(int, int, int, const void *, socklen_t);
int     shutdown(int, int);
int     sockatmark(int);
int     socket(int, int, int);
int     socketpair(int, int, int, int [2]);

#ifdef __cplusplus
}
#endif

#endif

#ifndef	_SYS_SOCKET_H
#define	_SYS_SOCKET_H

#include <inttypes.h>
#include <sys/types.h>
#include <sys/uio.h>

#define _SS_MAXSIZE	128

typedef size_t socklen_t;
typedef uint32_t sa_family_t;

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

/*** \todo cmsghdr ***/

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

enum SocketOptions
{
  SO_ACCEPTCONN = 0,
  SO_BROADCAST,
  SO_DEBUG,
  SO_DONTROUTE,
  SO_ERROR,
  SO_KEEPALIVE,
  SO_LINGER,
  SO_OOBINLINE,
  SO_RCVBUF,
  SO_RCVLOWAT,
  SO_RCVTIMEO,
  SO_REUSEADDR,
  SO_SNDBUF,
  SO_SNDLOWAT,
  SO_SNDTIMEO,
  SO_TYPE,
  SO_MAX
};

#define SOMAXCONN     65536

#define MSG_CTRUNC    0
#define MSG_DONTROUTE 1
#define MSG_EOR       2
#define MSG_OOB       4
#define MSG_NOSIGNAL  8
#define MSG_PEEK      16
#define MSG_TRUNC     32
#define MSG_WAITALL   64

#define PF_INET   0
#define PF_INET6  1
#define PF_UNIX   2
#define PF_SOCKET 3
#define PF_MAX    3
#define PF_UNSPEC (PF_MAX+1)

#define AF_INET   (PF_INET)
#define AF_INET6  (PF_INET6)
#define AF_UNIX   (PF_UNIX)
#define AF_UNSPEC (PF_MAX+1)

#define  SHUT_RD 0 /** Disables receiving */
#define  SHUT_WR 1 /** Disables sending */
#define  SHUT_RDWR 2 /** Disables recv/send */

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

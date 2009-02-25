#ifndef	_SYS_SOCKET_H
#define	_SYS_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SYS_SOCK_CONSTANTS_ONLY

#include <inttypes.h>
#include <sys/types.h>
//#include <sys/uio.h>

typedef size_t socklen_t;
typedef uint32_t sa_family_t;

struct sockaddr
{
  sa_family_t sa_family;
  char sa_data[14];
};

/// \todo sockaddr_storage structure

struct msghdr
{
  void*         msg_name;
  socklen_t     msg_namelen;
  //struct iovec* msg_iov;
  void*         msg_iov;
  int32_t       msg_iovlen;
  void*         msg_control;
  socklen_t     msg_controllen;
  int32_t       msg_flags;
};

/// \todo cmsghdr

struct linger
{
  int l_onoff;
  int l_linger;
};

#endif

#define SOCK_DGRAM      0
#define SOCK_RAW        1
#define SOCK_STREAM     2
#define SOCK_SEQPACKET  3

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
  SO_TYPE
};

#define SOMAXCONN     65536

/// \todo check that these are actually exclusive, if not, set to bit values
enum MessageFlags
{
  MSG_CTRUNC = 0,
  MSG_DONTROUTE,
  MSG_EOR,
  MSG_OOB,
  MSG_NOSIGNAL,
  MSG_PEEK,
  MSG_TRUNC,
  MSG_WAITALL
};

enum AddressFamilies
{
  AF_INET = 0,
  AF_INET6,
  AF_UNSPEC
};

enum ShutdownTypes
{
  SHUT_RD = 0, // disables receiving
  SHUT_WR, // disables sending
  SHUT_RDWR // disables recv/send
};

#ifndef SYS_SOCK_CONSTANTS_ONLY
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
#endif


#ifdef __cplusplus
}
#endif

#endif

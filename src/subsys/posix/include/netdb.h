#ifndef _NETDB_H
#define _NETDB_H

#include <stdint.h>

#include <sys/socket.h>
#include <netinet/in.h>

struct hostent
{
  char* h_name;
  char** h_aliases;
  int h_addrtype;
  int h_length;
  char** h_addr_list;
  char* h_addr;
};

struct netent
{
  char* n_name;
  char** n_aliases;
  int n_addrtype;
  uint32_t n_net;
};

struct protoent
{
  char* p_name;
  char** p_aliases;
  int p_proto;
};

struct servent
{
  char* s_name;
  char** s_aliases;
  int s_port;
  char* s_proto;
};

struct addrinfo
{
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    socklen_t ai_addrlen;
    struct sockaddr *ai_addr;
    char *ai_canonname;
    struct addrinfo *ai_next;
};

extern int h_errno;

#define HOST_NOT_FOUND    1
#define NO_DATA           2
#define NO_RECOVERY       3
#define TRY_AGAIN         4
#define NO_ADDRESS        5

#define AI_PASSIVE      0
#define AI_CANONNAME    1
#define AI_NUMERICHOST  2
#define AI_NUMERICSERV  4
#define AI_V4MAPPED     8
#define AI_ALL          16
#define AI_ADDRCONFIG   32

#define NI_NOFQDN       0
#define NI_NUMERICHOST  1
#define NI_NAMEREQD     2
#define NI_NUMERICSERV  4
#define NI_NUMERICSCOPE 8
#define NI_DGRAM        16

#define EAI_AGAIN       0
#define EAI_BADFLAGS    1
#define EAI_FAIL        2
#define EAI_FAMILY      3
#define EAI_MEMORY      4
#define EAI_NONAME      5
#define EAI_SERVICE     6
#define EAI_SOCKTYPE    7
#define EAI_SYSTEM      8
#define EAI_OVERFLOW    9

#ifdef __cplusplus
extern "C" {
#endif

const char      *gai_strerror(int ecode);

void             endhostent(void);
void             endnetent(void);
void             endprotoent(void);
void             endservent(void);
struct hostent  *gethostbyaddr(const void *addr, size_t len, int type);
struct hostent  *gethostbyname(const char *name);
struct hostent  *gethostent(void);
struct netent   *getnetbyaddr(uint32_t net, int type);
struct netent   *getnetbyname(const char *name);
struct netent   *getnetent(void);
struct protoent *getprotobyname(const char *name);
struct protoent *getprotobynumber(int proto);
struct protoent *getprotoent(void);
struct servent  *getservbyname(const char *name, const char *proto);
struct servent  *getservbyport(int port, const char *proto);
struct servent  *getservent(void);
void             sethostent(int stayopen);
void             setnetent(int stayopen);
void             setprotoent(int stayopen);
void             setservent(int stayopen);
void             freeaddrinfo(struct addrinfo *ai);
int              getaddrinfo(const char *nodename, const char *servname, const struct addrinfo *hints, struct addrinfo **res);
int              getnameinfo(const struct sockaddr *sa, socklen_t salen, char *node, socklen_t nodelen, char *service,
                             socklen_t servicelen, int flags);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif

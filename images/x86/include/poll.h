#ifndef _POLL_H_
#define _POLL_H_


struct pollfd
{
  int fd;
  short events;
  short revents;
};

typedef unsigned int nfds_t;

#define POLLRDNORM    1
#define POLLRDBAND    2
#define POLLPRI       4
#define POLLWRNORM    8
#define POLLWRBAND    16
#define POLLERR       32
#define POLLHUP       64
#define POLLNVAL      128

#define POLLOUT       (POLLWRNORM)
#define POLLIN        (POLLRDNORM | POLLRDBAND)

int poll(struct pollfd fds[], nfds_t nfds, int timeout);

#endif


/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

#include "syscallNumbers.h"

#include "errno.h"
#define errno (*__errno())
extern int *__errno (void);
int h_errno; // required by networking code

// Define errno before including syscall.h.
#include "syscall.h"

#include "newlib.h"

// Needs to be seperated, otherwise the compiler's time.h is used.
#include "include/time.h"

#define BS8(x) (x)
#define BS16(x) (((x&0xFF00)>>8)|((x&0x00FF)<<8))
#define BS32(x) (((x&0xFF000000)>>24)|((x&0x00FF0000)>>8)|((x&0x0000FF00)<<8)|((x&0x000000FF)<<24))
#define BS64(x) (x)

#ifdef LITTLE_ENDIAN

#define LITTLE_TO_HOST8(x) (x)
#define LITTLE_TO_HOST16(x) (x)
#define LITTLE_TO_HOST32(x) (x)
#define LITTLE_TO_HOST64(x) (x)

#define HOST_TO_LITTLE8(x) (x)
#define HOST_TO_LITTLE16(x) (x)
#define HOST_TO_LITTLE32(x) (x)
#define HOST_TO_LITTLE64(x) (x)

#define BIG_TO_HOST8(x) BS8((x))
#define BIG_TO_HOST16(x) BS16((x))
#define BIG_TO_HOST32(x) BS32((x))
#define BIG_TO_HOST64(x) BS64((x))

#define HOST_TO_BIG8(x) BS8((x))
#define HOST_TO_BIG16(x) BS16((x))
#define HOST_TO_BIG32(x) BS32((x))
#define HOST_TO_BIG64(x) BS64((x))

#else // else Big endian

#define BIG_TO_HOST8(x) (x)
#define BIG_TO_HOST16(x) (x)
#define BIG_TO_HOST32(x) (x)
#define BIG_TO_HOST64(x) (x)

#define HOST_TO_BIG8(x) (x)
#define HOST_TO_BIG16(x) (x)
#define HOST_TO_BIG32(x) (x)
#define HOST_TO_BIG64(x) (x)

#define LITTLE_TO_HOST8(x) BS8((x))
#define LITTLE_TO_HOST16(x) BS16((x))
#define LITTLE_TO_HOST32(x) BS32((x))
#define LITTLE_TO_HOST64(x) BS64((x))

#define HOST_TO_LITTLE8(x) BS8((x))
#define HOST_TO_LITTLE16(x) BS16((x))
#define HOST_TO_LITTLE32(x) BS32((x))
#define HOST_TO_LITTLE64(x) BS64((x))

#endif

// #define MAXNAMLEN 255

#define STUBBED(str) syscall1(POSIX_STUBBED, (int)(str)); \
  errno = ENOSYS;

#if 0
#define	F_DUPFD		0	/* Duplicate fildes */
#define	F_GETFD		1	/* Get fildes flags (close on exec) */
#define	F_SETFD		2	/* Set fildes flags (close on exec) */
#define	F_GETFL		3	/* Get file flags */
#define	F_SETFL		4	/* Set file flags */
#endif

//char *tzname[2] = { (char *)"GMT", (char *)"GMT" };
int daylight = 0;
long timezone = 0;
int altzone = 0;

int ftruncate(int a, off_t b)
{
    STUBBED("ftruncate");
    return -1;
}

char* getcwd(char *buf, unsigned long size)
{
    return (char *)syscall2(POSIX_GETCWD, (int) buf, (int) size);
}

int mkdir(const char *p, mode_t mode)
{
    return (int)syscall2(POSIX_MKDIR, (int)p, mode);
}

int close(int file)
{
    return (int)syscall1(POSIX_CLOSE, file);
}

int _execve(char *name, char **argv, char **env)
{
    return (int)syscall3(POSIX_EXECVE, (int)name, (int)argv, (int)env);
}

void _exit(int val)
{
    syscall1(POSIX_EXIT, val);
    while (1);
}

int fork()
{
    return (int)syscall0(POSIX_FORK);
}

int vfork()
{
    return fork();
}

int fstat(int file, struct stat *st)
{
    return (int)syscall2(POSIX_FSTAT, (int)file, (int)st);
}

int getpid()
{
    return (int)syscall0(POSIX_GETPID);
}

int _isatty(int file)
{
    return (int) syscall1(POSIX_ISATTY, file);
}

int link(const char *old, const char *_new)
{
    return (int)syscall2(POSIX_LINK, (int) old, (int) _new);
}

off_t lseek(int file, off_t ptr, int dir)
{
    return (off_t) syscall3(POSIX_LSEEK, file, ptr, dir);
}

int open(const char *name, int flags, ...) // , mode_t mode)
{
    va_list ap;
    va_start(ap, flags);
    mode_t mode = va_arg(ap, mode_t);
    va_end(ap);
    return (int)syscall3(POSIX_OPEN, (int)name, flags, mode);
}

_ssize_t read(int file, void *ptr, size_t len)
{
    return (_ssize_t) syscall3(POSIX_READ, file, (int)ptr, len);
}

void *sbrk(ptrdiff_t incr)
{
    return (void*) syscall1(POSIX_SBRK, incr);
}

int stat(const char *file, struct stat *st)
{
    return (int)syscall2(POSIX_STAT, (int)file, (int)st);
}

#ifndef PPC_COMMON
int times(void *buf)
{
    STUBBED("times");
    errno = ENOSYS;
    return -1;
}
#else
/* PPC has times() defined in terms of getrusage. */
int getrusage(int target, void *buf)
{
    STUBBED("getrusage");
    return -1;
}
#endif

int unlink(const char *name)
{
    return (int)syscall1(POSIX_UNLINK, (int)name);
}

int wait(int *status)
{
    return waitpid(-1, status, 0);
}

int waitpid(int pid, int *status, int options)
{
    return (int)syscall3(POSIX_WAITPID, pid, (int)status, options);
}

_ssize_t write(int file, const void *ptr, size_t len)
{
    return (_ssize_t) syscall3(POSIX_WRITE, file, (int)ptr, len);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
    if(!iov || !iovcnt || (fd == -1))
    {
        errno = EINVAL;
        return -1;
    }

    ssize_t ret = 0;
    int i;
    for(i = 0; i < iovcnt; i++)
    {
        if(iov[i].iov_base)
        {
            ssize_t r = read(fd, iov[i].iov_base, iov[i].iov_len);
            ret += r;
        }
    }
    return ret;
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    if(!iov || !iovcnt || (fd == -1))
    {
        errno = EINVAL;
        return -1;
    }

    ssize_t ret = 0;
    int i;
    for(i = 0; i < iovcnt; i++)
    {
        if(iov[i].iov_base)
        {
            ssize_t r = write(fd, iov[i].iov_base, iov[i].iov_len);
            ret += r;
        }
    }
    return ret;
}

int lstat(const char *file, struct stat *st)
{
    return (int) syscall2(POSIX_LSTAT, (int)file, (int)st);
}

DIR *opendir(const char *dir)
{
    DIR *p = (DIR*) malloc(sizeof(DIR));
    p->fd = syscall2(POSIX_OPENDIR, (int)dir, (int)&p->ent);
    if (p->fd < 0)
    {
        free(p);
        return 0;
    }
    return p;
}

struct dirent *readdir(DIR *dir)
{
    if (!dir)
        return 0;

    if (syscall2(POSIX_READDIR, dir->fd, (int)&dir->ent) != -1)
        return &dir->ent;
    else
        return 0;
}

void rewinddir(DIR *dir)
{
    if (!dir)
        return;

    syscall2(POSIX_REWINDDIR, dir->fd, (int)&dir->ent);
}

int closedir(DIR *dir)
{
    if (!dir)
        return 0;

    syscall1(POSIX_CLOSEDIR, dir->fd);
    free(dir);
    return 0;
}

int rename(const char *old, const char *new)
{
    return (int)syscall2(POSIX_RENAME, (int) old, (int) new);
}

int tcgetattr(int fd, struct termios *p)
{
    return (int)syscall2(POSIX_TCGETATTR, fd, (int)p);
}

int tcsetattr(int fd, int optional_actions, struct termios *p)
{
    return (int)syscall3(POSIX_TCSETATTR, fd, optional_actions, (int)p);
}

int mkfifo(const char *_path, mode_t __mode)
{
    STUBBED("mkfifo");
    return -1;
}

int gethostname(char *name, size_t len)
{
    STUBBED("gethostname");
    strcpy(name, "pedigree");
    return 0;
}

int sethostname(char *name, int len)
{
    STUBBED("sethostname");
    return 0;
}

int ioctl(int fd, int command, void *buf)
{
    return (int)syscall3(POSIX_IOCTL, fd, command, (int)buf);
}

int tcflow(int fd, int action)
{
    STUBBED("tcflow");
    return 0;
}

int tcflush(int fd, int queue_selector)
{
    STUBBED("tcflush");
    return 0;
}

int tcdrain(int fd)
{
    STUBBED("tcdrain");
    return -1;
}

int gettimeofday(struct timeval *tv, void *tz)
{
    syscall2(POSIX_GETTIMEOFDAY, (int)tv, (int)tz);

    return 0;
}

uid_t getuid()
{
    return (int)syscall0(POSIX_GETUID);
}

gid_t getgid()
{
    return (int)syscall0(POSIX_GETGID);
}

uid_t geteuid()
{
    STUBBED("geteuid");
    return getuid();
}

gid_t getegid()
{
    STUBBED("getegid");
    return getgid();
}

const char * const sys_siglist[] =
{
    0,
    "Hangup",
    "Interrupt",
    "Quit",
    "Illegal instruction",
    "Trap",
    "IOT",
    "Abort",
    "EMT",
    "Floating point exception",
    "Kill",
    "Bus error",
    "Segmentation violation",
    "Bad argument to system call",
    "Pipe error",
    "Alarm",
    "Terminate"
};

char *strsignal(int sig)
{
    if (sig < 16)
        return (char*)sys_siglist[sig];
    else
        return (char*)"Unknown";
}

int setuid(uid_t uid)
{
    STUBBED("setuid");
    errno = EINVAL;
    return -1;
}

int setgid(gid_t gid)
{
    STUBBED("setgid");
    errno = EINVAL;
    return -1;
}

unsigned int sleep(unsigned int seconds)
{
    return (unsigned int)syscall1(POSIX_SLEEP, seconds);
}

int usleep(useconds_t useconds)
{
    // Microseconds, TODO: support better granularity than 1 second
    return (unsigned int)syscall1(POSIX_SLEEP, useconds / 1000000);
}

unsigned int alarm(unsigned int seconds)
{
    return (unsigned int)syscall1(POSIX_ALARM, seconds);
}

mode_t umask(mode_t mask)
{
    STUBBED("umask");
    return 0;
}

int chmod(const char *path, mode_t mode)
{
    STUBBED("chmod");
    return 0;

    errno = ENOENT;
    return -1;
}

int chown(const char *path, uid_t owner, gid_t group)
{
    STUBBED("chown");
    return 0;

    errno = ENOENT;
    return -1;
}

int utime(const char *path,const struct utimbuf *times)
{
    STUBBED("utime");
    return 0;

    errno = ENOENT;
    return -1;
}

int access(const char *path, int amode)
{
    return (int) syscall2(POSIX_ACCESS, (int) path, amode);
}

const char * const sys_errlist[] = {};
const int sys_nerr = 0;
long timezone;

long pathconf(const char *path, int name)
{
    STUBBED("pathconf");
    return 0;
}

long fpathconf(int filedes, int name)
{
    STUBBED("fpathconf");
    return 0;
}

int cfgetospeed(const struct termios *t)
{
    STUBBED("cfgetospeed");
    return 0;
}

int cfgetispeed(const struct termios *t)
{
    STUBBED("cfgetispeed");
    return 0;
}

int cfsetospeed(const struct termios *t, int speed)
{
    STUBBED("cfsetospeed");
    return 0;
}

int cfsetispeed(const struct termios *t, int speed)
{
    STUBBED("cfsetispeed");
    return 0;
}


int select(int nfds, struct fd_set * readfds,
           struct fd_set * writefds, struct fd_set * errorfds,
           struct timeval * timeout)
{
    return (int)syscall5(POSIX_SELECT, nfds, (int)readfds, (int)writefds, (int)errorfds, (int)timeout);
}

void setgrent()
{
    STUBBED("setgrent");
}

void endgrent()
{
    STUBBED("endgrent");
}

struct group *getgrent()
{
    STUBBED("getgrent");
    errno = ENOSYS;
    return 0;
}

static struct passwd g_passwd;
int g_passwd_num = 0;
char g_passwd_str[256];
void setpwent()
{
    g_passwd_num = 0;
}

void endpwent()
{
    g_passwd_num = 0;
}

struct passwd *getpwent()
{
    if (syscall3(POSIX_GETPWENT, (int)&g_passwd, g_passwd_num, (int)&g_passwd_str) != 0)
        return 0;
    g_passwd_num++;
    return &g_passwd;
}

struct passwd *getpwuid(uid_t uid)
{
    if (syscall3(POSIX_GETPWENT, (int)&g_passwd, uid, (int)&g_passwd_str) != 0)
        return 0;
    return &g_passwd;
}

struct passwd *getpwnam(const char *name)
{
    if (syscall3(POSIX_GETPWNAM, (int)&g_passwd, (int)name, (int)&g_passwd_str) != 0)
        return 0;
    return &g_passwd;
}

// Pedigree-specific function: login with given uid and password.
int login(uid_t uid, char *password)
{
    return (int)syscall2(PEDIGREE_LOGIN, uid, (int)password);
}

int chdir(const char *path)
{
    return (int)syscall1(POSIX_CHDIR, (int)path);
}

int dup(int fileno)
{
    return (int)syscall1(POSIX_DUP, fileno);
}

int dup2(int fildes, int fildes2)
{
    return (int)syscall2(POSIX_DUP2, fildes, fildes2);
}

int pipe(int filedes[2])
{
    return (int)syscall1(POSIX_PIPE, (int) filedes);
}

int fcntl(int fildes, int cmd, ...)
{
    va_list ap;
    va_start(ap, cmd);

    int num = 0;
    int* args = 0;
    switch (cmd)
    {
        // only one argument for each of these
        case F_DUPFD:
        case F_SETFD:
        case F_SETFL:
            args = (int*) malloc(sizeof(int));
            args[0] = va_arg(ap, int);
            num = 1;
            break;
        case F_GETLK:
        case F_SETLK:
        case F_SETLKW:
            args = (int*) malloc(sizeof(struct flock*));
            args[0] = (int) va_arg(ap, struct flock*);
            num = 1;
            break;
    };
    va_end(ap);

    int ret = syscall4(POSIX_FCNTL, fildes, cmd, num, (int) args);

    if (args)
        free(args);
    return ret;
}

int sigprocmask(int how, const sigset_t* set, sigset_t* oset)
{
    return (int)syscall3(POSIX_SIGPROCMASK, how, (int) set, (int) oset);
}

int fchown(int fildes, uid_t owner, uid_t group)
{
    STUBBED("fchown");
    return 0;

    return -1;
}

int rmdir(const char *path)
{
    STUBBED("rmdir");
    return -1;
}

int socket(int domain, int type, int protocol)
{
    return (int)syscall3(POSIX_SOCKET, domain, type, protocol);
}

int connect(int sock, const struct sockaddr* address, size_t addrlen)
{
    return (int)syscall3(POSIX_CONNECT, sock, (int) address, (int) addrlen);
}

ssize_t send(int sock, const void * buff, size_t bufflen, int flags)
{
    return (ssize_t)syscall4(POSIX_SEND, sock, (int) buff, (int) bufflen, flags);
}

ssize_t recv(int sock, void * buff, size_t bufflen, int flags)
{
    return (ssize_t)syscall4(POSIX_RECV, sock, (int) buff, (int) bufflen, flags);
}

int accept(int sock, struct sockaddr* remote_addr, size_t *addrlen)
{
    return (int)syscall3(POSIX_ACCEPT, sock, (int) remote_addr, (int) addrlen);
}

int bind(int sock, const struct sockaddr* local_addr, size_t addrlen)
{
    return (int)syscall3(POSIX_BIND, sock, (int) local_addr, (int) addrlen);
}

int getpeername(int sock, struct sockaddr* addr, size_t *addrlen)
{
    STUBBED("getpeername");
    return -1;
}

int getsockname(int sock, struct sockaddr* addr, size_t *addrlen)
{
    STUBBED("getsockname");

    struct sockaddr_in *a = (struct sockaddr_in *) addr;
    a->sin_family = AF_INET;
    a->sin_port = 0;
    a->sin_addr.s_addr = 0;

    *addrlen = sizeof(struct sockaddr_in);

    return 0;
}

int getsockopt(int sock, int level, int optname, void* optvalue, size_t *optlen)
{
    STUBBED("getsockopt");
    return -1;
}

int listen(int sock, int backlog)
{
    return (int)syscall2(POSIX_LISTEN, sock, backlog);
}

struct special_send_recv_data
{
    int sock;
    void* buff;
    size_t bufflen;
    int flags;
    struct sockaddr* remote_addr;
    socklen_t* addrlen;
} __attribute__((packed));

ssize_t recvfrom(int sock, void* buff, size_t bufflen, int flags, struct sockaddr* remote_addr, size_t *addrlen)
{
    struct special_send_recv_data* tmp = (struct special_send_recv_data*) malloc(sizeof(struct special_send_recv_data));
    tmp->sock = sock;
    tmp->buff = buff;
    tmp->bufflen = bufflen;
    tmp->flags = flags;
    tmp->remote_addr = remote_addr;
    tmp->addrlen = addrlen;

    int ret = syscall1(POSIX_RECVFROM, (int) tmp);

    free(tmp);

    return ret;
}

long recvmsg(int sock, struct msghdr* msg, int flags)
{
    STUBBED("recvmsg");
    return -1;
}

long sendmsg(int sock, const struct msghdr* msg, int flags)
{
    STUBBED("sendmsg");
    return -1;
}

ssize_t sendto(int sock, const void* buff, size_t bufflen, int flags, const struct sockaddr* remote_addr, socklen_t addrlen)
{
    struct special_send_recv_data* tmp = (struct special_send_recv_data*) malloc(sizeof(struct special_send_recv_data));
    tmp->sock = sock;
    tmp->buff = (char *)buff;
    tmp->bufflen = bufflen;
    tmp->flags = flags;
    tmp->remote_addr = (struct sockaddr*)remote_addr;
    tmp->addrlen = &addrlen;

    int ret = syscall1(POSIX_SENDTO, (int) tmp);

    free(tmp);

    return ret;
}

int setsockopt(int sock, int level, int optname, const void* optvalue, unsigned long optlen)
{
    STUBBED("setsockopt");
    return 0;
}

int shutdown(int sock, int how)
{
    return (int) syscall2(POSIX_SHUTDOWN, sock, how);
}

int sockatmark(int sock)
{
    STUBBED("sockatmark");
    return -1;
}

int socketpair(int domain, int type, int protocol, int sock_vec[2])
{
    STUBBED("socketpair");
    return -1;
}

unsigned int inet_addr(const char *cp)
{
    /// \todo Support formats other than a.b.c.d
    /// \todo Rewrite!

    char* tmp = (char*) malloc(strlen(cp));
    char* tmp_ptr = tmp; // so we can free the memory
    strcpy(tmp, (char *)cp);

    // iterate through, removing decimals and taking the four pointers
    char* elements[4] = {tmp, 0, 0, 0};
    int num = 1;
    while (*tmp)
    {
        if (*tmp == '.')
        {
            *tmp = 0;
            elements[num++] = tmp + 1;
        }
        else if(!isdigit(*tmp))
        {
            // Not a digit - IPs must be digits and periods only!
            free(tmp_ptr);
            return -1;
        }

        tmp++;

    }

    if (num != 4)
        return 0;

    unsigned int a = atoi(elements[0]);
    unsigned int b = atoi(elements[1]);
    unsigned int c = atoi(elements[2]);
    unsigned int d = atoi(elements[3]);

    unsigned int ret = (d << 24) | (c << 16) | (b << 8) | a;

    free(tmp_ptr);

    return ret;
}

char* inet_ntoa(struct in_addr addr)
{
    static char buff[16];
    sprintf(buff, "%u.%u.%u.%u", addr.s_addr & 0xff, (addr.s_addr & 0xff00) >> 8, (addr.s_addr & 0xff0000) >> 16, (addr.s_addr & 0xff000000) >> 24);
    return buff;
}

int inet_aton(const char *cp, struct in_addr *inp)
{
    return inet_addr(cp);
}

struct hostent* gethostbyaddr(const void *addr, unsigned long len, int type)
{
    static struct hostent ret;
    if (syscall4(POSIX_GETHOSTBYADDR, (int) addr, len, type, (int) &ret) != 0)
        return &ret;
    return 0;
}

struct hostent* gethostbyname(const char *name)
{
    static struct hostent* ret = 0;
    if (ret == 0)
        ret = (struct hostent*) malloc(512);
    if (ret == 0)
        return (struct hostent*) 0;

    int success = syscall3(POSIX_GETHOSTBYNAME, (int) name, (int) ret, 512);
    if (success == 0)
    {
        ret->h_addr = ret->h_addr_list[0];
        return ret;
    }
    else
        return (struct hostent*) 0;
}

struct servent* getservbyname(const char *name, const char *proto)
{
    STUBBED("getservbyname");
    return 0;
}

void endservent()
{
    STUBBED("endservent");
}

struct servent* getservbyport(int port, const char *proto)
{
    STUBBED("setservbyport");
    return 0;
}

struct servent* getservent()
{
    STUBBED("setservent");
    return 0;
}

void setservent(int stayopen)
{
    STUBBED("setservent");
}

void endprotoent()
{
    STUBBED("endprotoent");
}

struct protoent* getprotobyname(const char *name)
{
    static struct protoent* ent = 0;
    if (ent == 0)
    {
        ent = (struct protoent*) malloc(512);
        ent->p_name = 0;
    }
    if (ent->p_name)
        free(ent->p_name);

    ent->p_name = (char*) malloc(strlen(name) + 1);
    ent->p_aliases = 0;
    strcpy(ent->p_name, (char*)name);

    if (!strcmp(name, "icmp"))
        ent->p_proto = IPPROTO_ICMP;
    else if (!strcmp(name, "udp"))
        ent->p_proto = IPPROTO_UDP;
    else if (!strcmp(name, "tcp"))
        ent->p_proto = IPPROTO_TCP;
    else
        ent->p_proto = 0;

    return ent;
}

struct protoent* getprotobynumber(int proto)
{
    STUBBED("getprotobynumber");
    return 0;
}

struct protoent* getprotoent()
{
    STUBBED("getprotoent");
    return 0;
}

void setprotoent(int stayopen)
{
    STUBBED("setprotoent");
}

int getgrnam()
{
    STUBBED("getgrnam");
    return 0;
}

int getgrgid()
{
    STUBBED("getgrgid");
    return 0;
}

int symlink(const char *path1, const char *path2)
{
    STUBBED("symlink");
    return 0;
}

int fsync(int fd)
{
    STUBBED("fsync");
    return 0;
}

int inet_pton()
{
    STUBBED("inet_pton");
    return -1;
}

const char* inet_ntop(int af, const void* src, char* dst, unsigned long size)
{
    STUBBED("inet_ntop");
    return 0;
}

ssize_t readlink(const char* path, char* buf, size_t bufsize)
{
    return (int) syscall3(POSIX_READLINK, (int) path, (int) buf, bufsize);
}

int ftime(struct timeb *tp)
{
    STUBBED("ftime");
    return -1;
}

int sigmask()
{
    STUBBED("sigmask");
    return -1;
}

int sigblock()
{
    STUBBED("sigblock");
    return -1;
}

int sigsetmask(int mask)
{
    STUBBED("sigsetmask");
    return -1;
}

int siggetmask()
{
    STUBBED("siggetmask");
    return -1;
}

int sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
    return (int)syscall3(POSIX_SIGACTION, sig, (int) act, (int) oact);
}

_sig_func_ptr signal(int s, _sig_func_ptr func)
{
    // obtain the old mask for the sigaction structure, fill it in with default arguments
    // and pass on to sigaction
    static struct sigaction act;
    static struct sigaction tmp;
    unsigned long mask = 0;
    sigprocmask(0, 0, &mask);
    act.sa_mask = mask;
    act.sa_handler = func;
    act.sa_flags = 0;
    memset(&tmp, 0, sizeof(struct sigaction));
    if (sigaction(s, &act, &tmp) == 0)
    {
        return tmp.sa_handler;
    }

    // errno set by sigaction
    return (_sig_func_ptr) -1;
}

int raise(int sig)
{
    return (int)syscall1(POSIX_RAISE, sig);
}

int kill(pid_t pid, int sig)
{
    return (int)syscall2(POSIX_KILL, pid, sig);
}

int sigpending(long* set)
{
    STUBBED("sigpending");
    return -1;
}

int sigsuspend(const long* sigmask)
{
    STUBBED("sigsuspend");
    return -1;
}

void _init_signals()
{
    syscall0(PEDIGREE_INIT_SIGRET);
}

int fdatasync(int fildes)
{
    STUBBED("fdatasync");
    return -1;
}

struct dlHandle
{
    int mode;
};

void *dlopen(const char *file, int mode)
{
    void* p = (void*) malloc(sizeof(struct dlHandle));
    if (!p)
        return 0;
    void* ret = (void*) syscall3(POSIX_DLOPEN, (int) file, mode, (int) p);
    if (ret)
        return ret;
    free(p);
    return 0;
}

void *dlsym(void* handle, const char* name)
{
    return (void*) syscall2(POSIX_DLSYM, (int) handle, (int) name);
}

int dlclose(void *handle)
{
    STUBBED("dlclose");
    if (handle)
        free(handle);
    return 0;
}

char *dlerror()
{
    STUBBED("dlerror");
    return 0;
}

int poll(struct pollfd fds[], unsigned int nfds, int timeout)
{
    return (int)syscall3(POSIX_POLL, (int)fds, nfds, timeout);
}

void herror(const char *s)
{
    char *buff = (char*)strerror(h_errno);
    printf("%s: %s\n", s, buff);
}

int makedev()
{
    STUBBED("makedev");
    return -1;
}

unsigned int htonl(unsigned int n)
{
    return HOST_TO_BIG32(n);
}
unsigned int ntohl(unsigned int n)
{
    return BIG_TO_HOST32(n);
}

unsigned short htons(unsigned short n)
{
    return HOST_TO_BIG16(n);
}
unsigned short ntohs(unsigned short n)
{
    return BIG_TO_HOST16(n);
}

int fchmod(int fildes, mode_t mode)
{
    STUBBED("fchmod");
    return -1;
}

void sync()
{
    STUBBED("sync");
}

/// \todo This is pretty hacky, might want to run a syscall instead and have it populated
///       in the kernel where we can change it as time goes on.
int uname(struct utsname *n)
{
    if (!n)
        return -1;
    strcpy(n->sysname, "Pedigree");
    strcpy(n->release, "Foster");
    strcpy(n->version, "0.1");
    strcpy(n->machine, "i686");
    gethostname(n->nodename, 128);
    return 0;
}

int mknod(const char *path, mode_t mode, dev_t dev)
{
    STUBBED("mknod");
    return -1;
}

/// \todo open needs to work on directories, then implement this
int fchdir(int fildes)
{
    STUBBED("fchdir");
    return -1;
}

int getpwuid_r(uid_t uid, struct passwd *pwd, char *buffer, size_t bufsize, struct passwd **result)
{
    STUBBED("getpwuid_r");
    return -1;
}

int getgrgid_r(gid_t gid, struct group *grp, char *buffer, size_t bufsize, struct group **result)
{
    STUBBED("getgrgid_r");
    return -1;
}

int getpwnam_r(const char *name, struct passwd *pwd, char *buffer, size_t bufsize, struct passwd **result)
{
    STUBBED("getpwnam_r");
    return -1;
}

int getgrnam_r(const char *name, struct group *grp, char *buffer, size_t bufsize, struct group **result)
{
    STUBBED("getgrnam_r");
    return -1;
}

void err(int eval, const char * fmt, ...)
{
    printf("err: %d: (todo: print format string based on arguments): %s\n", errno, strerror(errno));
    exit(eval);
}

void freeaddrinfo(struct addrinfo *ai)
{
    if(ai)
    {
        free(ai->ai_canonname);
        struct addrinfo *tmp;
        for(tmp = ai; tmp != 0;)
        {
            ai = tmp;
            tmp = ai->ai_next;

            if(ai->ai_next)
                free(ai->ai_next);
        }
    }

    errno = 0;
}

/// \todo Hacked implementation to get Pacman working. Needs to be improved!
///       Biggest improvement would be moving into the kernel
int getaddrinfo(const char *nodename, const char *servname, const struct addrinfo *hints, struct addrinfo **res)
{
    // Validate incoming arguments
    if(!res)
    {
        errno = EINVAL;
        return EAI_SYSTEM;
    }

    // Return buffer
    struct addrinfo *ret = (struct addrinfo*) malloc(sizeof(struct addrinfo));
    if(!ret)
    {
        errno = ENOMEM;
        return EAI_SYSTEM;
    }

    // Our static sockaddr for returning
    static struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    if(servname)
    {
        addr.sin_port = htons(atoi(servname));
    }

    // Fill the basics of the return pointer
    if(hints)
        *ret = *hints;
    else
    {
        ret->ai_flags = 0;
        ret->ai_socktype = SOCK_STREAM;
        ret->ai_protocol = 0;
    }
    ret->ai_family = PF_INET;

    // Attempt to turn the node name into an IP
    int ip = 0;
    if(nodename)
        ip = inet_addr(nodename);
    else
        ip = inet_addr("127.0.0.1");
    struct hostent *h;
    if(ip == -1)
    {
        if(!nodename)
            return EAI_FAIL;

        // Not an IP... Try a DNS lookup
        STUBBED(nodename);
        h = gethostbyname(nodename);
        if(!h)
            return EAI_FAIL;

        memcpy(&(addr.sin_addr.s_addr), h->h_addr, h->h_length);
        ret->ai_addrlen = h->h_length;
    }
    else
    {
        // It's an IP
        memcpy(&(addr.sin_addr.s_addr), &ip, 4);
        ret->ai_addrlen = 4;
    }

    // Fill the rest now
    ret->ai_addr = (struct sockaddr*) &addr;
    if(nodename)
    {
        ret->ai_canonname = (char*) malloc(strlen(nodename) + 1);
        strcpy(ret->ai_canonname, nodename);
    }
    else
    {
        ret->ai_canonname = (char*) malloc(strlen("localhost") + 1);
        strcpy(ret->ai_canonname, "localhost");
    }
    ret->ai_next = 0;

    // Tell the caller where the pointer is at
    *res = ret;

    return 0;
}

int getnameinfo(const struct sockaddr *sa, socklen_t salen, char *node, socklen_t nodelen, char *service, socklen_t servicelen, int flags)
{
    STUBBED("getnameinfo");
    return -1;
}

long timegm(struct tm *tm)
{
    STUBBED("timegm");
    return -1;
}

int chroot(const char *path)
{
    STUBBED("chroot");
    return -1;
}

char *mkdtemp(char *template)
{
    STUBBED("mkdtemp");
    return 0;
}

const char * const gai_strings[] =
{
    "The name could not be resolved at this time.", // EAI_AGAIN
    "The flags had an invalid value.", // EAI_BADFLAGS
    "A non-recoverable error occurred.", // EAI_FAIL
    "The address family was not recognized or the address length was invalid for the specified family.", // EAI_FAMILY
    "There was a memory allocation failure.", // EAI_MEMORY
    "The name does not resolve for the supplied parameters.", // EAI_NONAME
    "The service passed was not recognized for the specified socket type.", // EAI_SERVICE
    "The intended socket type was not recognized.", // EAI_SOCKTYPE
    "A system error occurred (see errno).", // EAI_SYSTEM
    "An argument buffer overflowed." // EAI_OVERFLOW
};

const char *gai_strerror(int ecode)
{
    if (ecode > EAI_OVERFLOW)
        return "";
    return gai_strings[ecode];
}

int setitimer(int which, const struct itimerval *value, struct itimerval *ovalue)
{
    STUBBED("setitimer");
    return -1;
}

struct _mmap_tmp
{
    void *addr;
    size_t len;
    int prot;
    int flags;
    int fildes;
    off_t off;
};

void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    struct _mmap_tmp t;
    t.addr = addr;
    t.len = len;
    t.prot = prot;
    t.flags = flags;
    t.fildes = fildes;
    t.off = off;

    return (void*) syscall1(POSIX_MMAP, (int) &t);
}

int munmap(void *addr, size_t len)
{
    return (int) syscall2(POSIX_MUNMAP, (int) addr, (int) len);
}

int getgroups(int gidsetsize, gid_t grouplist[])
{
    if(gidsetsize == 0)
    {
        return 1;
    }
    else
    {
        if(!grouplist)
        {
            errno = EINVAL;
            return -1;
        }

        grouplist[0] = getgid();
        return 1;
    }
}

size_t getpagesize()
{
    return sysconf(_SC_PAGESIZE);
}

char *realpath(const char *file_name, char *resolved_name)
{
    STUBBED("realpath");
    if(resolved_name && file_name)
    {
        strcpy(resolved_name, file_name);
        return resolved_name;
    }

    errno = EINVAL;
    return 0;
}

pid_t setsid()
{
    return syscall0(POSIX_SETSID);
}

int setpgid(pid_t pid, pid_t pgid)
{
    return syscall2(POSIX_SETPGID, (int) pid, (int) pgid);
}

pid_t getpgrp()
{
    return syscall0(POSIX_GETPGRP);
}

pid_t getppid()
{
    STUBBED("getppid");
    return 0;
}

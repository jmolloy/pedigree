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

#include "posixSyscallNumbers.h"

// Define errno before including syscall.h.
#include "errno.h"
#define errno (*__errno())
extern int *__errno (void);
int h_errno; // required by networking code

#include "posix-syscall.h"

#include "newlib.h"

#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <semaphore.h>

#include <sys/resource.h>
#include <sys/statfs.h>

#include <setjmp.h>

#include <pedigree_config.h>

#define PEDIGREE_SYSCALLS_LIBC
#include <pedigree-syscalls.h>

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

#define STUBBED(str) syscall1(POSIX_STUBBED, (long)(str)); \
    errno = ENOSYS;

#define NUM_ATFORK_HANDLERS 32 // (* 3)

// For getopt(3).
int optreset = 0;

// Defines an fork handler
struct forkHandler
{
    void (*prepare)(void);
    void (*parent)(void);
    void (*child)(void);
};

// Tables of handlers
static struct forkHandler atforkHandlers[NUM_ATFORK_HANDLERS];

// Number of handlers (also an index to the next one)
static int nHandlers = 0;

int ftruncate(int a, off_t b)
{
	return syscall2(POSIX_FTRUNCATE, a, (long) b);
}

char* getcwd(char *buf, unsigned long size)
{
    // Because the developers of bash can't be assed following a specification,
    // we have to make this dirty hack so that getcwd with a null buffer works.
    // Even though that's technically unspecified behaviour that should be an
    // error condition.
    if(!buf)
        buf = (char*) malloc(size ? size : PATH_MAX);
    return (char *)syscall2(POSIX_GETCWD, (long) buf, (long) size);
}

int mkdir(const char *p, mode_t mode)
{
    return (long)syscall2(POSIX_MKDIR, (long)p, mode);
}

int close(int file)
{
    return (long)syscall1(POSIX_CLOSE, file);
}

int _execve(char *name, char **argv, char **env)
{
    return (long)syscall3(POSIX_EXECVE, (long)name, (long)argv, (long)env);
}

void _exit(int val)
{
    syscall1(POSIX_EXIT, val);
    while (1);
}

int fork(void)
{
    if(nHandlers)
    {
        for(int i = 0; i < nHandlers; i++)
        {
            if(atforkHandlers[i].prepare)
                atforkHandlers[i].prepare();
        }
    }

    int pid = (long)syscall0(POSIX_FORK);

    if(pid == 0)
    {
        if(nHandlers)
        {
            for(int i = 0; i < nHandlers; i++)
            {
                if(atforkHandlers[i].child)
                    atforkHandlers[i].child();
            }
        }
    }
    else if(pid > 0)
    {
        if(nHandlers)
        {
            for(int i = 0; i < nHandlers; i++)
            {
                if(atforkHandlers[i].parent)
                    atforkHandlers[i].parent();
            }
        }
    }

    return pid;
}

int vfork(void)
{
    return fork();
}

int fstat(int file, struct stat *st)
{
    return (long)syscall2(POSIX_FSTAT, (long)file, (long)st);
}

int getpid(void)
{
    return (long)syscall0(POSIX_GETPID);
}

int _isatty(int file)
{
    return (long) syscall1(POSIX_ISATTY, file);
}

int link(const char *old, const char *_new)
{
    return (long)syscall2(POSIX_LINK, (long) old, (long) _new);
}

off_t lseek(int file, off_t ptr, int dir)
{
    return (off_t) syscall3(POSIX_LSEEK, file, ptr, dir);
}

int open(const char *name, int flags, ...) // , mode_t mode)
{
    // Try to handle invalid arguments early, before we go to the effort of the
    // system call...
    if(!name)
    {
        errno = EINVAL;
        return -1;
    }
    va_list ap;
    va_start(ap, flags);
    mode_t mode = va_arg(ap, mode_t);
    va_end(ap);
    return (long)syscall3(POSIX_OPEN, (long)name, flags, mode);
}

_ssize_t read(int file, void *ptr, size_t len)
{
    if(file < 0)
    {
        syslog(LOG_NOTICE, "[%d] read: bad file given\n", getpid());
        errno = EBADF;
        return -1;
    }
    if(!ptr)
    {
        syslog(LOG_NOTICE, "[%d] read: bad buffer given\n", getpid());
        return 0;
    }
    if(len == 0)
    {
        syslog(LOG_NOTICE, "[%d] read: bad length given\n", getpid());
        return 0;
    }
    return (_ssize_t) syscall3(POSIX_READ, file, (long)ptr, len);
}

void *sbrk(ptrdiff_t incr)
{
    return (void*) syscall1(POSIX_SBRK, incr);
}

int stat(const char *file, struct stat *st)
{
    return (long)syscall2(POSIX_STAT, (long)file, (long)st);
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
    return (long)syscall1(POSIX_UNLINK, (long)name);
}

int wait(int *status)
{
    return waitpid(-1, status, 0);
}

int waitpid(int pid, int *status, int options)
{
    return (long)syscall3(POSIX_WAITPID, pid, (long)status, options);
}

_ssize_t write(int file, const void *ptr, size_t len)
{
    if(file < 0)
    {
        syslog(LOG_NOTICE, "[%d] write: bad file given\n", getpid());
        errno = EBADF;
        return -1;
    }
    return (_ssize_t) syscall3(POSIX_WRITE, file, (long)ptr, len);
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
    return (long) syscall2(POSIX_LSTAT, (long)file, (long)st);
}

DIR *opendir(const char *dir)
{
    DIR *p = (DIR*) malloc(sizeof(DIR));
    p->fd = syscall2(POSIX_OPENDIR, (long)dir, (long)&p->ent);
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

    if (syscall2(POSIX_READDIR, dir->fd, (long)&dir->ent) != -1)
        return &dir->ent;
    else
        return 0;
}

void rewinddir(DIR *dir)
{
    if (!dir)
        return;

    syscall2(POSIX_REWINDDIR, dir->fd, (long)&dir->ent);
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
    return (long)syscall2(POSIX_RENAME, (long) old, (long) new);
}

int tcgetattr(int fd, struct termios *p)
{
    return (long)syscall2(POSIX_TCGETATTR, fd, (long)p);
}

int tcsetattr(int fd, int optional_actions, struct termios *p)
{
    return (long)syscall3(POSIX_TCSETATTR, fd, optional_actions, (long)p);
}

int mkfifo(const char *_path, mode_t __mode)
{
    STUBBED("mkfifo");
    return -1;
}

int gethostname(char *name, size_t len)
{
    int result;

    if(!name || !len)
        return -1;

    result = pedigree_config_query("select * from 'network-generic' WHERE `key` = 'hostname';");
    if((result == -1) || (pedigree_config_was_successful(result) == -1) || (pedigree_config_numrows(result) == 0))
    {
        if(result != -1)
            pedigree_config_freeresult(result);
        strncpy(name, "pedigree", len);
        return 0;
    }

    pedigree_config_getstr_s(result, 0, "value", name, len);
    pedigree_config_freeresult(result);
    return 0;
}

int	sethostname(char *name, size_t len)
{
    if(!name || len > 255 || !len)
    {
        errno = EINVAL;
        return -1;
    }

    // Need to add permission and name checking

    const char *query = "update 'network-generic' set `value`= '%s' WHERE `key` = 'hostname'";
    char *tmp    = pedigree_config_escape_string(name);
    char *buffer = (char*)malloc(strlen(query) + strlen(tmp) - 2 + 1);

    sprintf(buffer,query,tmp);

    int result = pedigree_config_query(buffer);

    if(result != -1)
        pedigree_config_freeresult(result);

    free(tmp);
    free(buffer);

    return 0;
}

/// \todo ioctl is actually supposed to take varargs, not a single buffer
int ioctl(int fd, int command, void *buf)
{
    return (long)syscall3(POSIX_IOCTL, fd, command, (long)buf);
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
    syscall2(POSIX_GETTIMEOFDAY, (long)tv, (long)tz);

    return 0;
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

uid_t getuid(void)
{
    return (long)syscall0(POSIX_GETUID);
}

gid_t getgid(void)
{
    return (long)syscall0(POSIX_GETGID);
}

uid_t geteuid(void)
{
    return (uid_t) syscall0(POSIX_GETEUID);
}

gid_t getegid(void)
{
    return (gid_t) syscall0(POSIX_GETEGID);
}

int setuid(uid_t uid)
{
    return syscall1(POSIX_SETUID, uid);
}

int setgid(gid_t gid)
{
    return syscall1(POSIX_SETGID, gid);
}

int seteuid(uid_t uid)
{
    return syscall1(POSIX_SETEUID, uid);
}

int setegid(gid_t gid)
{
    return syscall1(POSIX_SETEGID, gid);
}

int setresuid(uid_t ruid, uid_t euid, uid_t suid)
{
    STUBBED("setresuid");
    return -1;
}

int setresgid(gid_t rgid, gid_t egid, gid_t sgid)
{
    STUBBED("setresgid");
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
    return syscall2(POSIX_CHMOD, (long) path, mode);
}

int chown(const char *path, uid_t owner, gid_t group)
{
    return syscall3(POSIX_CHOWN, (long) path, owner, group);
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
    return (long) syscall2(POSIX_ACCESS, (long) path, amode);
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
    return (long)syscall5(POSIX_SELECT, nfds, (long)readfds, (long)writefds, (long)errorfds, (long)timeout);
}

void setgrent(void)
{
    STUBBED("setgrent");
}

void endgrent(void)
{
    STUBBED("endgrent");
}

struct group *getgrent(void)
{
    STUBBED("getgrent");
    errno = ENOSYS;
    return 0;
}

static struct passwd g_passwd;
int g_passwd_num = 0;
char g_passwd_str[256];
void setpwent(void)
{
    g_passwd_num = 0;
}

void endpwent(void)
{
    g_passwd_num = 0;
}

struct passwd *getpwent(void)
{
    if (syscall3(POSIX_GETPWENT, (long)&g_passwd, g_passwd_num, (long)&g_passwd_str) != 0)
        return 0;
    g_passwd_num++;
    return &g_passwd;
}

struct passwd *getpwuid(uid_t uid)
{
    if (syscall3(POSIX_GETPWENT, (long)&g_passwd, uid, (long)&g_passwd_str) != 0)
        return 0;
    return &g_passwd;
}

struct passwd *getpwnam(const char *name)
{
    if (syscall3(POSIX_GETPWNAM, (long)&g_passwd, (long)name, (long)&g_passwd_str) != 0)
        return 0;
    return &g_passwd;
}

int chdir(const char *path)
{
    return (long)syscall1(POSIX_CHDIR, (long)path);
}

int dup(int fileno)
{
    return (long)syscall1(POSIX_DUP, fileno);
}

int dup2(int fildes, int fildes2)
{
    return (long)syscall2(POSIX_DUP2, fildes, fildes2);
}

int pipe(int filedes[2])
{
    return (long)syscall1(POSIX_PIPE, (long) filedes);
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
            args = (int*) malloc(sizeof(long));
            args[0] = va_arg(ap, int);
            num = 1;
            break;
        case F_GETLK:
        case F_SETLK:
        case F_SETLKW:
            args = (int*) malloc(sizeof(struct flock*));
            args[0] = (long) va_arg(ap, struct flock*);
            num = 1;
            break;
    };
    va_end(ap);

    int ret = syscall4(POSIX_FCNTL, fildes, cmd, num, (long) args);

    if (args)
        free(args);
    return ret;
}

int sigprocmask(int how, const sigset_t* set, sigset_t* oset)
{
    return (long)syscall3(POSIX_SIGPROCMASK, how, (long) set, (long) oset);
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
    return (long)syscall3(POSIX_SOCKET, domain, type, protocol);
}

int connect(int sock, const struct sockaddr* address, size_t addrlen)
{
    return (long)syscall3(POSIX_CONNECT, sock, (long) address, (long) addrlen);
}

ssize_t send(int sock, const void * buff, size_t bufflen, int flags)
{
    return (ssize_t)syscall4(POSIX_SEND, sock, (long) buff, (long) bufflen, flags);
}

ssize_t recv(int sock, void * buff, size_t bufflen, int flags)
{
    return (ssize_t)syscall4(POSIX_RECV, sock, (long) buff, (long) bufflen, flags);
}

int accept(int sock, struct sockaddr* remote_addr, size_t *addrlen)
{
    return (long)syscall3(POSIX_ACCEPT, sock, (long) remote_addr, (long) addrlen);
}

int bind(int sock, const struct sockaddr* local_addr, size_t addrlen)
{
    return (long)syscall3(POSIX_BIND, sock, (long) local_addr, (long) addrlen);
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
    return (long)syscall2(POSIX_LISTEN, sock, backlog);
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

    int ret = syscall1(POSIX_RECVFROM, (long) tmp);

    free(tmp);

    return ret;
}

ssize_t recvmsg(int sock, struct msghdr* msg, int flags)
{
    STUBBED("recvmsg");
    return -1;
}

ssize_t sendmsg(int sock, const struct msghdr* msg, int flags)
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

    int ret = syscall1(POSIX_SENDTO, (long) tmp);

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
    return (long) syscall2(POSIX_SHUTDOWN, sock, how);
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

#define INET_ADDR_INVALID   ((in_addr_t)(-1))

int inet_addr(const char *cp)
{
    syslog(LOG_NOTICE, "[%d] inet_addr(%s)", getpid(), (cp ? cp : "<invalid>"));

    // Valid string?
    if(!cp || !*cp)
        return INET_ADDR_INVALID;

    // Reallocate the string so the memory can be modified
    char* tmp = (char*) malloc(strlen(cp) + 1);
    strcpy(tmp, (char *)cp);

    // Store the pointer so the memory can be freed
    char* tmp_ptr = tmp;

    // Is there a non-IP character in the string?
    size_t c, max = strlen(tmp);
    for(c = 0; c < max; c++)
    {
        // Only digits and periods in IPs
        char test = tmp[c];
        if(!isdigit(test) && (test != '.'))
        {
            free(tmp_ptr);
            return INET_ADDR_INVALID;
        }
    }

    // Does the string have a '.' at all?
    if(strchr(tmp, '.') != 0)
    {
        // Build the list, at most there will be 4
        char *ipComponents[4] = {tmp, 0, 0, 0};
        int numComponents = 1;
        while((tmp = strchr(tmp, '.')) != 0)
        {
            // Terminate this component
            *tmp = '\0';

            // Add to the list
            ipComponents[numComponents++] = ++tmp;
        }

        // Handle
        int ret = 0, i;
        switch(numComponents)
        {
            case 4:
                // Standard case
                for(i = 0; i < 4; i++)
                    ret += (atoi(ipComponents[i]) & 0xFF) << ((3 - i) * 8);
                break;
            case 3:
                {
                    // Last quantity is a 16-bit value
                    int a = atoi(ipComponents[0]);
                    int b = atoi(ipComponents[1]);
                    int c = atoi(ipComponents[2]);
                    ret = ((a & 0xFF) << 24) + ((b & 0xFF) << 16) + (c & 0xFFFF);
                }
                break;

            case 2:
                {
                    // Last quantity is a 16-bit value
                    int a = atoi(ipComponents[0]);
                    int b = atoi(ipComponents[1]);
                    ret = ((a & 0xFF) << 24) + (b & 0xFFFFFF);
                }
                break;

            // This shouldn't be reachable, but you never know
            case 1:
            default:
                ret = INET_ADDR_INVALID;
        }

        // All done - return in network byte order
        free(tmp_ptr);
        return htonl(ret);
    }
    else
    {
        // Convert to an integer and return in network byte order
        int ret = atoi(tmp);
        free(tmp_ptr);
        return htonl(ret);
    }
}


char* inet_ntoa(struct in_addr addr)
{
    static char buff[32];
    sprintf(buff, "%u.%u.%u.%u", addr.s_addr & 0xff, (addr.s_addr & 0xff00) >> 8, (addr.s_addr & 0xff0000) >> 16, (addr.s_addr & 0xff000000) >> 24);
    return buff;
}

int inet_aton(const char *cp, struct in_addr *inp)
{
    in_addr_t ip = inet_addr(cp);
    if(ip != INET_ADDR_INVALID)
    {
        inp->s_addr = ip;
        return ip;
    }
    return 0;
}

struct hostent* gethostbyaddr(const void *addr, unsigned long len, int type)
{
    syslog(LOG_NOTICE, "[%d] gethostbyaddr", getpid());

    static struct hostent ret;
    if (syscall4(POSIX_GETHOSTBYADDR, (long) addr, len, type, (long) &ret) != 0)
        return &ret;
    return 0;
}

struct hostent* gethostbyname(const char *name)
{
    syslog(LOG_NOTICE, "[%d] gethostbyname(%s)", getpid(), (name ? name : "<invalid>"));

    static struct hostent* ret = 0;
    if (ret == 0)
        ret = (struct hostent*) malloc(512);
    if (ret == 0)
        return (struct hostent*) 0;

    int success = syscall3(POSIX_GETHOSTBYNAME, (long) name, (long) ret, 512);
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
    if(!name)
        return 0;

    static struct servent se;
    static int first = 1;
    if(first)
    {
        se.s_name = 0;
        se.s_aliases = 0;
        se.s_proto = 0;
        first = 0;
    }
    char buf[256], *escName = pedigree_config_escape_string(name), *escProto;

    if(proto)
    {
        escProto = pedigree_config_escape_string(proto);
        sprintf(buf, "select * from 'network-services' where name = '%s' and proto = '%s'", escName, escProto);
        free(escProto);
    }
    else
        sprintf(buf, "select * from 'network-services' where name = '%s'", escName);

    free(escName);

    int result = pedigree_config_query(buf);
    if (result == -1 || pedigree_config_was_successful(result) || !pedigree_config_numrows(result))
    {

        if(result != -1)
            pedigree_config_freeresult(result);
        return 0;
    }

    if(se.s_name)
        free(se.s_name);
    if(se.s_proto)
        free(se.s_proto);

    se.s_name = (char*) malloc(64);
    char *newProto = (char*) malloc(64);
    if(proto)
        strncpy(newProto, proto, 64);
    else
        pedigree_config_getstr_s(result, 0, "proto", newProto, 64);

    se.s_port = pedigree_config_getnum_s(result, 0, "port");
    strncpy(se.s_name, name, 64);
    se.s_proto = newProto;

    pedigree_config_freeresult(result);

    return &se;
}

void endservent(void)
{
    STUBBED("endservent");
}

struct servent* getservbyport(int port, const char *proto)
{
    static struct servent se;
    static int first = 1;
    if(first)
    {
        se.s_name = 0;
        se.s_aliases = 0;
        se.s_proto = 0;
        first = 0;
    }
    char buf[256];

    if(proto)
        sprintf(buf, "select * from 'network-services' where port = %d and proto = '%s'", port, pedigree_config_escape_string(proto));
    else
        sprintf(buf, "select * from 'network-services' where port = %d", port);

    int result = pedigree_config_query(buf);
    if (result == -1 || pedigree_config_was_successful(result) || !pedigree_config_numrows(result))
    {
        if(result != -1)
            pedigree_config_freeresult(result);
        return 0;
    }

    se.s_name = (char*) malloc(64);
    char *newProto = (char*) malloc(64);
    if(proto)
        strncpy(newProto, proto, 64);
    else
        pedigree_config_getstr_s(result, 0, "proto", newProto, 64);

    pedigree_config_getstr_s(result, 0, "name", se.s_name, 64);
    se.s_port = port;
    se.s_proto = newProto;

    pedigree_config_freeresult(result);

    return &se;
}

struct servent* getservent(void)
{
    STUBBED("setservent");
    return 0;
}

void setservent(int stayopen)
{
    STUBBED("setservent");
}

void endprotoent(void)
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

struct protoent* getprotoent(void)
{
    STUBBED("getprotoent");
    return 0;
}

void setprotoent(int stayopen)
{
    STUBBED("setprotoent");
}

struct group *getgrnam(const char *name)
{
    STUBBED("getgrnam");

    /// \todo HACK HACK HACKITY HACK SO VERY HACKY. Yeah, this is for Apache.
    /// Also it's going to leak memory everywhere, and use it really badly.

    static struct group ret;

    ret.gr_name = (char*) malloc(128);
    strcpy(ret.gr_name, name);

    ret.gr_gid = 3; // httpd

    ret.gr_mem = (char**) malloc(8);
    ret.gr_mem[0] = (char*) malloc(128);
    strcpy(ret.gr_mem[0], "httpd");
    ret.gr_mem[1] = 0;

    return &ret;
}

struct group *getgrgid(gid_t id)
{
    STUBBED("getgrgid");
    return 0;
}

int symlink(const char *path1, const char *path2)
{
    return (long) syscall2(POSIX_SYMLINK, (long)path1, (long) path2);
}

int fsync(int fd)
{
    STUBBED("fsync");
    return 0;
}

int inet_pton(void)
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
    return (long) syscall3(POSIX_READLINK, (long) path, (long) buf, bufsize);
}

int ftime(struct timeb *tp)
{
    STUBBED("ftime");
    return -1;
}

int sigmask(void)
{
    STUBBED("sigmask");
    return -1;
}

int sigblock(void)
{
    STUBBED("sigblock");
    return -1;
}

int sigsetmask(int mask)
{
    STUBBED("sigsetmask");
    return -1;
}

int siggetmask(void)
{
    STUBBED("siggetmask");
    return -1;
}

int sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
    return (long)syscall3(POSIX_SIGACTION, sig, (long) act, (long) oact);
}

_sig_func_ptr signal(int s, _sig_func_ptr func)
{
    // Obtain the old mask for the sigaction structure, fill it in with default arguments
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
    return (long)syscall1(POSIX_RAISE, sig);
}

int kill(pid_t pid, int sig)
{
    return (long)syscall2(POSIX_KILL, pid, sig);
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

void _init_signals(void)
{
    // syscall0(PEDIGREE_INIT_SIGRET);
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
    void* ret = (void*) syscall3(POSIX_DLOPEN, (long) file, mode, (long) p);
    if (ret)
        return ret;
    free(p);
    return 0;
}

void *dlsym(void* handle, const char* name)
{
    return (void*) syscall2(POSIX_DLSYM, (long) handle, (long) name);
}

int dlclose(void *handle)
{
    STUBBED("dlclose");
    if (handle)
        free(handle);
    return 0;
}

char *dlerror(void)
{
    STUBBED("dlerror");
    return 0;
}

int poll(struct pollfd fds[], unsigned int nfds, int timeout)
{
    return (long)syscall3(POSIX_POLL, (long)fds, nfds, timeout);
}

#define HOST_NOT_FOUND    1
#define NO_DATA           2
#define NO_RECOVERY       3
#define TRY_AGAIN         4
#define NO_ADDRESS        5

const char * const sys_herrors[] =
{
    0,
    "The host cannot be found.",
    "The requested name is valid, but does not have an IP address."
    "A non-recoverable name server error occurred.",
    "A temporary error occurred on an authoritative name server. Try again later.",
    "The requested name is valid, but does not have an IP address."
};

const char *hstrerror(int err)
{
    return sys_herrors[err];
}

void herror(const char *s)
{
    const char *buff = hstrerror(h_errno);
    printf("%s: %s\n", s, buff);
}

int makedev(void)
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

void sync(void)
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
    syslog(LOG_NOTICE, "[%d] getaddrinfo(%s, %s, %x, %x)", getpid(),
                                                           (nodename ? nodename : "<invalid>"),
                                                           (servname ? servname : "<invalid>"),
                                                           hints,
                                                           res
                                                           );

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
        addr.sin_port = atoi(servname);

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
    in_addr_t ip = 0;
    if(nodename)
        ip = inet_addr(nodename);
    else
        ip = inet_addr("127.0.0.1");
    struct hostent *h;
    if(ip == ((in_addr_t)(-1)))
    {
        if(!nodename)
        {
            free(ret);
            return EAI_FAIL;
        }

        // Not an IP... Try a DNS lookup
        h = gethostbyname(nodename);
        if(!h)
        {
            free(ret);
            return EAI_FAIL;
        }

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

int getitimer(int which, struct itimerval *value)
{
    STUBBED("getitimer");
    return -1;
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

    return (void*) syscall1(POSIX_MMAP, (long) &t);
}

int munmap(void *addr, size_t len)
{
    return (long) syscall2(POSIX_MUNMAP, (long) addr, (long) len);
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

size_t getpagesize(void)
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

pid_t setsid(void)
{
    return syscall0(POSIX_SETSID);
}

int setpgid(pid_t pid, pid_t pgid)
{
    return syscall2(POSIX_SETPGID, (long) pid, (long) pgid);
}

pid_t getpgrp(void)
{
    return syscall0(POSIX_GETPGRP);
}

pid_t getppid(void)
{
    STUBBED("getppid");
    return 0;
}

int getrlimit(int resource, struct rlimit *rlp)
{
    STUBBED("setrlimit");
    return -1;
}

int setrlimit(int resource, const struct rlimit *rlp)
{
    STUBBED("setrlimit");
    return -1;
}

/// \todo Write - should just be a simple ls-style read of the raw drive
int getmntinfo(struct statfs **mntbufp, int flags)
{
    STUBBED("getmntinfo");
    return -1;
}

int statfs(const char *path, struct statfs *buf)
{
    STUBBED("statfs");
    return -1;
}

int fstatfs(int fd, struct statfs *buf)
{
    STUBBED("fstatfs");
    return -1;
}

struct fstab *getfsent(void)
{
    STUBBED("getfsent");
    return 0;
}

struct fstab *getfsfile(const char *mount_point)
{
    STUBBED("getfsfile");
    return 0;
}

struct fstab *getfsspec(const char *special_file)
{
    STUBBED("getfsspec");
    return 0;
}

int setfsent(void)
{
    STUBBED("setfsent");
    return -1;
}

void endfsent(void)
{
    STUBBED("endfsent");
}

int getrusage(int who, struct rusage *r_usage)
{
    STUBBED("getrusage");
    return -1;
}

int sigaltstack(const struct stack_t *stack, struct stack_t *oldstack)
{
    return syscall2(POSIX_SIGALTSTACK, (long) stack, (long) oldstack);
}

int sem_close(sem_t *sem)
{
    return syscall1(POSIX_SEM_CLOSE, (long) sem);
}

int sem_destroy(sem_t *sem)
{
    return syscall1(POSIX_SEM_DESTROY, (long) sem);
}

int sem_getvalue(sem_t *sem, int *val)
{
    return syscall2(POSIX_SEM_GETVALUE, (long) sem, (long) val);
}

int sem_init(sem_t *sem, int pshared, unsigned value)
{
    return syscall3(POSIX_SEM_INIT, (long) sem, pshared, value);
}

sem_t *sem_open(const char *name, int mode, ...)
{
    STUBBED("sem_open");
    return 0;
}

int sem_post(sem_t *sem)
{
    return syscall1(POSIX_SEM_POST, (long) sem);
}

int sem_timedwait(sem_t *sem, const struct timespec *tm)
{
    return syscall2(POSIX_SEM_TIMEWAIT, (long) sem, (long) tm);
}

int sem_trywait(sem_t *sem)
{
    return syscall1(POSIX_SEM_TRYWAIT, (long) sem);
}

int sem_unlink(const char *name)
{
    STUBBED("sem_unlink");
    return -1;
}

int sem_wait(sem_t *sem)
{
    return syscall1(POSIX_SEM_WAIT, (long) sem);
}

int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
    // Already full?
    if(nHandlers == NUM_ATFORK_HANDLERS)
    {
        errno = ENOMEM;
        return -1;
    }

    // Create and insert
    struct forkHandler handler;
    handler.prepare = prepare;
    handler.parent = parent;
    handler.child = child;
    atforkHandlers[nHandlers++] = handler;
    return 0;
}

void closelog()
{
}

void openlog(const char *log, int logopt, int facility)
{
}

int setlogmask(int mask)
{
    return 0;
}

void syslog(int prio, const char *fmt, ...)
{
    static char print_temp[1024];
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(print_temp, fmt, argptr);
    syscall2(POSIX_SYSLOG, (long) print_temp, prio);
    va_end(argptr);
}

int pause()
{
    STUBBED("pause");
    return -1;
}

pid_t forkpty(int *amaster, char *name, struct termios *termp, struct winsize *winp)
{
    STUBBED("forkpty");
    errno = ENOENT;
    return -1;
}

struct utmp *pututline(struct utmp *ut)
{
    STUBBED("pututline");
    return 0;
}

void logwtmp(const char *line, const char *name, const char *host)
{
    STUBBED("logwtmp");
}

unsigned if_nametoindex(const char *name)
{
    STUBBED("if_nametoindex");
    return 0;
}

char* if_indextoname(unsigned index, char *buf)
{
    STUBBED("if_indextoname");
    errno = ENXIO;
    return 0;
}

struct if_nameindex* if_nameindex()
{
    STUBBED("if_nameindex");
    errno = ENOBUFS;
    return 0;
}

void if_freenameindex(struct if_nameindex *nameindex)
{
    STUBBED("if_freenameindex");
}

int tcsendbreak(int fildes, int duration)
{
    STUBBED("tcsendbreak");
    return 0;
}

int pthread_cancel(pthread_t thread)
{
    STUBBED("pthread_cancel");
    return -1;
}

int sigsetjmp(sigjmp_buf env, int savemask)
{
    STUBBED("sigsetjmp");
    return -1;
}

void siglongjmp(sigjmp_buf env, int val)
{
    STUBBED("siglongjmp");
}

char *basename(char *path)
{
    static char bad[2] = {'.', 0};
    if(!path)
        return bad;
    char *p = strrchr(path, '/');
    if(!p)
        return path;
    else
        return p;
}

int reboot(int howto)
{
    /// \todo Check permissions, throw EPERM if can't reboot the system
    pedigree_reboot();

    // Technically we shouldn't actually get here...
    syslog(LOG_ERR, "pedigree_reboot returned?\n");
    return 0;
}

pid_t tcgetpgrp(int fildes)
{
    STUBBED("tcgetpgrp");
    return -1;
}

int initgroups(const char *user, gid_t group)
{
    STUBBED("initgroups");
    return 0;
}

int setgroups(int ngroups, const gid_t *gidset)
{
    STUBBED("setgroups");
    return 0;
}

ssize_t getdelim(char **a, size_t *b, int c, FILE *d)
{
    return __getdelim(a, b, c, d);
}
ssize_t getline(char **a, size_t *b, FILE *c)
{
    return __getline(a, b, c);
}

int sched_yield()
{
    return syscall0(POSIX_SCHED_YIELD);
}

int getdtablesize()
{
    STUBBED("getdtablesize");

    struct rlimit tmp;
    getrlimit(RLIMIT_NOFILE, &tmp);
    return tmp.rlim_cur;
}

int mprotect(void *addr, size_t len, int prot)
{
    STUBBED("mprotect");
    return 0;
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    if(!rqtp)
    {
        errno = EINVAL;
        return -1;
    }

    return syscall2(POSIX_NANOSLEEP, (long) rqtp, (long) rmtp);
}

int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    if(!tp)
    {
        errno = EINVAL;
        return -1;
    }

    return syscall2(POSIX_CLOCK_GETTIME, clock_id, (long) tp);
}

int setreuid(uid_t ruid, uid_t euid)
{
    STUBBED("setreuid");
    return 0;
}

int grantpt(int fildes)
{
    STUBBED("grantpt");
    return 0;
}

int unlockpt(int fildes)
{
    STUBBED("unlockpt");
    return 0;
}

char *ptsname(int fildes)
{
    STUBBED("ptsname");
    static char *ret = "[fake]";
    return ret;
}

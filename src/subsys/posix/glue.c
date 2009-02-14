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

//undef errno
//extern int errno; // <-- is right according to the newlib documentation, but isn't this racy across threads?

// Define errno before including syscall.h.
#include "syscall.h"

extern void *malloc(int);
extern void free(void*);
extern void strcpy(char*,char*);

extern void printf(char*,...);

#define MAXNAMLEN 255

struct dirent
{
  char d_name[MAXNAMLEN];
  int d_ino;
};

typedef struct ___DIR
{
  int fd;
  struct dirent ent;
} DIR;

struct timeval {
  unsigned int tv_sec;
  unsigned int tv_usec;
};

struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};

struct passwd {
	char	*pw_name;		/* user name */
	char	*pw_passwd;		/* encrypted password */
	int	pw_uid;			/* user uid */
	int	pw_gid;			/* user gid */
	char	*pw_comment;		/* comment */
	char	*pw_gecos;		/* Honeywell login info */
	char	*pw_dir;		/* home directory */
	char	*pw_shell;		/* default shell */
};

struct  stat
{
  int   st_dev;
  int   st_ino;
  int   st_mode;
  int   st_nlink;
  int   st_uid;
  int   st_gid;
  int   st_rdev;
  int   st_size;
  int   st_atime;
  int   st_mtime;
  int   st_ctime;
};

int ftruncate(int a, int b)
{
  errno = ENOSYS;
  return -1;
}

int mkdir(const char *p)
{
  errno = ENOSYS;
  return -1;
}

int close(int file)
{
  return syscall1(POSIX_CLOSE, file);
}

int _execve(char *name, char **argv, char **env)
{
  return syscall3(POSIX_EXECVE, name, argv, env);
}

void _exit(int val)
{
  syscall1(POSIX_EXIT, val);
}

int fork()
{
  int a = syscall0(POSIX_FORK);
  return a;
}

int fstat(int file, struct stat *st)
{
  return syscall2(POSIX_FSTAT, file, st);
}

int getpid()
{
  return syscall0(POSIX_GETPID);
}

int _isatty(int file)
{
//  errno = ENOSYS;
//  return -1;
  return 1;
}

int kill(int pid, int sig)
{
  errno = ENOSYS;
  return -1;
}

int link(char *old, char *_new)
{
  errno = ENOSYS;
  return -1;
}

int lseek(int file, int ptr, int dir)
{
  errno = ENOSYS;
  return -1;
}

int open(const char *name, int flags, int mode)
{
  return syscall3(POSIX_OPEN, name, flags, mode);
}

int read(int file, char *ptr, int len)
{
  return syscall3(POSIX_READ, file, ptr, len);
}

int sbrk(int incr)
{
  return syscall1(POSIX_SBRK, incr);
}

int stat(const char *file, struct stat *st)
{
  return syscall2(POSIX_STAT, file, st);
}

#ifndef PPC_COMMON
int times(void *buf)
{
  errno = ENOSYS;
  return -1;
}
#else
/* PPC has times() defined in terms of getrusage. */
int getrusage(int target, void *buf)
{
  errno = ENOSYS;
  return -1;
}
#endif

int unlink(char *name)
{
  errno = ENOSYS;
  return -1;
}

int wait(int *status)
{
  return waitpid(-1, status, 0);
}

int waitpid(int pid, int *status, int options)
{
  return syscall3(POSIX_WAITPID, pid, status, options);
}

int write(int file, char *ptr, int len)
{
  return syscall3(POSIX_WRITE, file, ptr, len);
}

int lstat(char *file, struct stat *st)
{
  return syscall2(POSIX_STAT, file, st);
}

DIR *opendir(const char *dir)
{
  DIR *p = malloc(sizeof(DIR));
  p->fd = syscall2(POSIX_OPENDIR, dir, &p->ent);
  return p;
}

struct dirent *readdir(DIR *dir)
{
  if (syscall2(POSIX_READDIR, dir->fd, &dir->ent) != -1)
    return &dir->ent;
  else
    return 0;
}

void rewinddir(DIR *dir)
{
  syscall2(POSIX_REWINDDIR, dir->fd, &dir->ent);
}

int closedir(DIR *dir)
{
  syscall1(POSIX_CLOSEDIR, dir->fd);
  free(dir);
  return 0;
}

int tcgetattr(int fd, void *p)
{
  return syscall2(POSIX_TCGETATTR, fd, p);
}

int tcsetattr(int fd, int optional_actions, void *p)
{
  return syscall3(POSIX_TCSETATTR, fd, optional_actions, p);
}

int mkfifo(const char *_path, int __mode)
{
  // Named pipes are not supported.
  errno = ENOSYS;
  return -1;
}

int gethostname(char *name, int len)
{
  /// \todo implement gethostname.
  strcpy(name, "pedigree");
  return 0;
}

int ioctl(int fd, int command, void *buf)
{
  return syscall3(POSIX_IOCTL, fd, command, buf);
}

int tcflow(int fd, int action)
{
//  errno = ENOSYS;
  return 0;
}

int tcflush(int fd, int queue_selector)
{
//  errno = ENOSYS;
  return 0;
}

int tcdrain(int fd)
{
  errno = ENOSYS;
  return -1;
}

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  /// \todo Implement.
  tv->tv_sec = 0;
  tv->tv_usec = 0;

  return 0;
}

long sysconf(int name)
{
  errno = EINVAL;
  return -1;
}

int getuid()
{
  /// \todo Implement.
  return 0;
}

int getgid()
{
  /// \todo Implement.
  return 0;
}

int geteuid()
{
  /// \todo Implement.
  return 0;
}

int getegid()
{
  /// \todo Implement.
  return 0;
}

int getppid()
{
  /// \todo Implement.
  return 0;
}

const char * const sys_siglist[] = {
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
  "Terminate" };

char *strsignal(int sig)
{
  if (sig < 16)
    return (char*)sys_siglist[sig];
  else
  return (char*)"Unknown";
}

int setuid(int uid)
{
  errno = EINVAL;
  return -1;
}

int setgid(int gid)
{
  errno = EINVAL;
  return -1;
}

unsigned int sleep(unsigned int seconds)
{
  /// \todo Implement
  return 0;
}

unsigned int alarm(unsigned int seconds)
{
  /// \todo Implement
  return 0;
}

int umask(int mask)
{
  /// \todo Implement
  return 0;
}

int chmod(const char *path, int mode)
{
  /// \todo Implement
  errno = ENOENT;
  return -1;
}

int chown(const char *path, int owner, int group)
{
  /// \todo Implement
  errno = ENOENT;
  return -1;
}

int utime(const char *path, const struct utimbuf *times)
{
  /// \todo Implement
  errno = ENOENT;
  return -1;
}

int sigaction(int sig, const struct sigaction *act, struct sigaction *oact)
{
  /// \todo Implement
  errno = ENOSYS;
  return -1;
}

int dup2(int fildes, int fildes2)
{
  errno = ENOENT;
  return -1;
}

int access(const char *path, int amode)
{
  /// \todo Implement
  errno = ENOENT;
  return 0;
}

const char * const sys_errlist[] = {};
const int sys_nerr = 0;
long timezone;

long pathconf(const char *path, int name)
{
  /// \todo Implement
  return 0;
}

long fpathconf(int filedes, int name)
{
  /// \todo Implement
  return 0;
}

int cfgetospeed(const struct termios *t)
{
  /// \todo Implement
  return 0;
}

int select(int nfds, struct fd_set * readfds,
                     struct fd_set * writefds, struct fd_set * errorfds,
                     struct timeval * timeout)
{
  return syscall5(POSIX_SELECT, nfds, readfds, writefds, errorfds, timeout);
}

void setgrent()
{
  /// \todo Implement
}

void endgrent()
{
  /// \todo Implement
}

struct group *getgrent()
{
  /// \todo Implement
  errno = ENOSYS;
  return 0;
}

int chdir(const char *path)
{
  return syscall1(POSIX_CHDIR, path);
}

int dup(int fileno)
{
  /// \todo Implement
  errno = ENOSYS;
  return -1;
}

int pipe(int filedes[2])
{
  /// \todo Implement
  errno = ENOSYS;
  return -1;
}

int fcntl(int fildes, int cmd, ...)
{
  /// \todo Implement
  errno = ENOSYS;
  return -1;
}

int sigprocmask(int how, int set, int oset)
{
  errno = ENOSYS;
  return -1;
}


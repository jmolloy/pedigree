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

#ifndef FILE_SYSCALLS_H
#define FILE_SYSCALLS_H

#include <vfs/File.h>

#define MAXNAMLEN 255

struct FileDescriptor
{
  File file;
  uint64_t offset;
};

struct dirent
{
  char d_name[MAXNAMLEN];
  short d_ino;
};

struct stat
{
  short   st_dev;
  unsigned short   st_ino;
  unsigned int   st_mode;
  unsigned short   st_nlink;
  unsigned short   st_uid;
  unsigned short   st_gid;
  short   st_rdev;
  unsigned long   st_size;
  int   st_atime;
  int   st_mtime;
  int   st_ctime;
};


/* Taken from the newlib source. Don't complain, just leave it... :( */

#  define _SYS_TYPES_FD_SET
#  define NBBY  8   /* number of bits in a byte */
/*
 * Select uses bit masks of file descriptors in longs.
 * These macros manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here
 * should be >= NOFILE (param.h).
 */
#  ifndef FD_SETSIZE
# define  FD_SETSIZE  64
#  endif

typedef long  fd_mask;
#  define NFDBITS (sizeof (fd_mask) * NBBY) /* bits per mask */
#  ifndef howmany
# define  howmany(x,y)  (((x)+((y)-1))/(y))
#  endif

/* We use a macro for fd_set so that including Sockets.h afterwards
   can work.  */
typedef struct _types_fd_set {
  fd_mask fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} _types_fd_set;

#define fd_set _types_fd_set

#  define FD_SET(n, p)  ((p)->fds_bits[(n)/NFDBITS] |= (1L << ((n) % NFDBITS)))
#  define FD_CLR(n, p)  ((p)->fds_bits[(n)/NFDBITS] &= ~(1L << ((n) % NFDBITS)))
#  define FD_ISSET(n, p)  ((p)->fds_bits[(n)/NFDBITS] & (1L << ((n) % NFDBITS)))
#  define FD_ZERO(p)  (__extension__ (void)({ \
     size_t __i; \
     char *__tmp = (char *)p; \
     for (__i = 0; __i < sizeof (*(p)); ++__i) \
       *__tmp++ = 0; \
}))

#define	S_IFCHR	0020000	/* character special */
#define S_IFDIR 0040000 /* Directory */
#define S_IFREG 0100000 /* Regular file */

/// \todo These should be time_t and suseconds_t!
struct timeval {
  unsigned long      tv_sec;
  unsigned long tv_usec;
};

int posix_close(int fd);
int posix_open(const char *name, int flags, int mode);
int posix_read(int fd, char *ptr, int len);
int posix_write(int fd, char *ptr, int len);
int posix_lseek(int file, int ptr, int dir);
int posix_link(char *old, char *_new);
int posix_unlink(char *name);
int posix_stat(const char *file, struct stat *st);
int posix_fstat(int fd, struct stat *st);
int posix_lstat(char *file, struct stat *st);

// Returns DIR->fd, takes &dir->ent.
int posix_opendir(const char *dir, dirent *ent);
int posix_readdir(int fd, dirent *ent);
void posix_rewinddir(int fd, dirent *ent);
int posix_closedir(int fd);

int posix_ioctl(int fd, int operation, void *buf);

int posix_chdir(const char *path);
int posix_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, timeval *timeout);

#endif

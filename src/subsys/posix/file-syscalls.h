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

#include <vfs/VFS.h>
#include <vfs/File.h>
#include <vfs/Filesystem.h>

#include "DevFs.h"

#include "newlib.h"

#if 1
#define F_NOTICE(x) NOTICE("[" << Dec << Processor::information().getCurrentThread()->getParent()->getId() << "]\t" << Hex << x)
#else
#define F_NOTICE(x)
#endif

#define MAXNAMLEN 255

class FileDescriptor
{
public:
  FileDescriptor () :
    file(), offset(0), fd(0xFFFFFFFF), fdflags(0), flflags(0)
  {
  }
  File* file;
  uint64_t offset;
  size_t fd;

  /// \todo What are these? Can they be documented please?
  int fdflags;
  int flflags;
};



/// \todo These should be time_t and suseconds_t!
// struct timeval {
//   unsigned long      tv_sec;
//   unsigned long tv_usec;
// };

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

int posix_dup(int fd);
int posix_dup2(int fd1, int fd2);

int posix_fcntl(int fd, int cmd, int num, int* args);

int posix_mkdir(const char* name, int mode);

#endif

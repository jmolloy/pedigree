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

struct  stat
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

#define	S_IFCHR	0020000	/* character special */
#define S_IFDIR 0040000 /* Directory */
#define S_IFREG 0100000 /* Regular file */

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

#endif

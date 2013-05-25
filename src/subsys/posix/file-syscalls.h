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

#if 0
#define F_NOTICE(x) NOTICE("[" << Dec << Processor::information().getCurrentThread()->getParent()->getId() << "]\t" << Hex << x)
#else
#define F_NOTICE(x)
#endif

#define MAXNAMLEN 255

int posix_close(int fd);
int posix_open(const char *name, int flags, int mode);
int posix_read(int fd, char *ptr, int len);
int posix_write(int fd, char *ptr, int len);
off_t posix_lseek(int file, off_t ptr, int dir);
int posix_link(char *old, char *_new);
int posix_unlink(char *name);
int posix_stat(const char *file, struct stat *st);
int posix_fstat(int fd, struct stat *st);
int posix_lstat(char *file, struct stat *st);
int posix_rename(const char* src, const char* dst);
int posix_symlink(char *target, char *link);

char* posix_getcwd(char* buf, size_t maxlen);
int posix_readlink(const char* path, char* buf, unsigned int bufsize);

// Returns DIR->fd, takes &dir->ent.
int posix_opendir(const char *dir, dirent *ent);
int posix_readdir(int fd, dirent *ent);
void posix_rewinddir(int fd, dirent *ent);
int posix_closedir(int fd);

int posix_ioctl(int fd, int operation, void *buf);

int posix_chmod(const char *path, mode_t mode);
int posix_chown(const char *path, uid_t owner, gid_t group);
int posix_fchmod(int fd, mode_t mode);
int posix_fchown(int fd, uid_t owner, gid_t group);
int posix_chdir(const char *path);
int posix_fchdir(int fd);

int posix_dup(int fd);
int posix_dup2(int fd1, int fd2);

int posix_fcntl(int fd, int cmd, int num, int* args);

int posix_mkdir(const char* name, int mode);

int posix_isatty(int fd);

void *posix_mmap(void *p);
int posix_msync(void *p, size_t len, int flags);
int posix_munmap(void *addr, size_t len);

int posix_access(const char *name, int amode);

int posix_ftruncate(int a, off_t b);

int posix_fstatvfs(int fd, struct statvfs *buf);
int posix_statvfs(const char *path, struct statvfs *buf);

#endif

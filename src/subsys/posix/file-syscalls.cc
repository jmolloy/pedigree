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

#include <syscallError.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <process/Process.h>
#include <utilities/Tree.h>
#include <vfs/File.h>
#include <vfs/Symlink.h>
#include <vfs/Directory.h>
#include <vfs/VFS.h>
#include <console/Console.h>
#include <network/NetManager.h>
#include <network/Tcp.h>
#include <utilities/utility.h>
#include <utilities/TimedTask.h>

#include <Subsystem.h>
#include <PosixSubsystem.h>

#include "file-syscalls.h"
#include "console-syscalls.h"
#include "pipe-syscalls.h"
#include "net-syscalls.h"

extern int posix_getpid();

//
// Syscalls pertaining to files.
//

typedef Tree<size_t,FileDescriptor*> FdMap;

#define GET_CWD() (Processor::information().getCurrentThread()->getParent()->getCwd())

int posix_close(int fd)
{
  F_NOTICE("close(" << fd << ")");
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }

  /// \todo Race here - fix.
  FdMap &fdMap = pSubsystem->getFdMap();
  FileDescriptor *f = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fd));

  if (!f)
  {
    SYSCALL_ERROR(BadFileDescriptor);
    return -1;
  }

  f->file->decreaseRefCount( (f->flflags & O_RDWR) || (f->flflags & O_WRONLY) );

  fdMap.remove(fd);
  delete f;
  return 0;
}

int posix_open(const char *name, int flags, int mode)
{
  F_NOTICE("open(" << name << ", " << ((mode&O_RDWR)?"O_RDWR":"") << ((mode&O_RDONLY)?"O_RDONLY":"") << ((mode&O_WRONLY)?"O_WRONLY":"") << ")");

  // verify the filename - don't try to open a dud file
  if(name[0] == 0)
  {
    SYSCALL_ERROR(DoesNotExist);
    return -1;
  }

  // Lookup this process.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }

  FdMap &fdMap = pSubsystem->getFdMap();

  size_t fd = pSubsystem->nextFd();

  // Check for /dev/tty, and link to our controlling console.
  File* file = 0;

  if (!strcmp(name, "/dev/tty"))
  {
    /// \todo Should be our ctty.
    file = ConsoleManager::instance().getConsole(String("console0"));
  }
  else if(!strcmp(name, "/dev/null"))
  {
    file = NullFs::instance().getFile();
  }
  else
  {
    file = VFS::instance().find(String(name), GET_CWD());
  }

  bool bCreated = false;
  if (!file)
  {
    if(flags & O_CREAT)
    {
      F_NOTICE("  {O_CREAT}");
      bool worked = VFS::instance().createFile(String(name), 0777, GET_CWD());
      if(!worked)
      {
        SYSCALL_ERROR(DoesNotExist);
        return -1;
      }

      file = VFS::instance().find(String(name), GET_CWD());
      if (!file)
      {
        SYSCALL_ERROR(DoesNotExist);
        return -1;
      }

      bCreated = true;
    }
    else
    {
        NOTICE("Does not exist.");
      // Error - not found.
      SYSCALL_ERROR(DoesNotExist);
      return -1;
    }
  }

  while (file->isSymlink())
    file = Symlink::fromFile(file)->followLink();

  if (file->isDirectory())
  {
    // Error - is directory.
        NOTICE("Is a directory.");
    SYSCALL_ERROR(IsADirectory);
    return -1;
  }

  if((flags & O_CREAT) && (flags & O_EXCL) && !bCreated)
  {
    // file exists with O_CREAT and O_EXCL
        NOTICE("File exists");
    SYSCALL_ERROR(FileExists);
    return -1;
  }

  if((flags & O_TRUNC) && ((flags & O_CREAT) || (flags & O_WRONLY) || (flags & O_RDWR)))
  {
    F_NOTICE("  {O_TRUNC}");
    // truncate the file
    file->truncate();
  }

  FileDescriptor *f = new FileDescriptor;
  f->file = file;
  f->offset = (flags & O_APPEND) ? file->getSize() : 0;
  f->fd = fd;
  f->fdflags = 0;
  f->flflags = flags | mode;
  fdMap.insert(fd, f);
  file->increaseRefCount(false);
  NOTICE("fine: " << fd);
  return static_cast<int> (fd);
}

int posix_read(int fd, char *ptr, int len)
{
  F_NOTICE("read(" << Dec << fd << ", " << Hex << reinterpret_cast<uintptr_t>(ptr) << ", " << len << ")");

  // Lookup this process.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }
  FdMap &fdMap = pSubsystem->getFdMap();

  FileDescriptor *pFd = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fd));
  if (!pFd)
  {
    // Error - no such file descriptor.
    SYSCALL_ERROR(BadFileDescriptor);
    return -1;
  }

  uint64_t nRead = 0;
  if(ptr && len)
  {
    char *kernelBuf = new char[len];
    nRead = pFd->file->read(pFd->offset, len, reinterpret_cast<uintptr_t>(kernelBuf));
    memcpy(reinterpret_cast<void*>(ptr), reinterpret_cast<void*>(kernelBuf), len);
    delete [] kernelBuf;

    pFd->offset += nRead;
  }

  return static_cast<int>(nRead);
}

int posix_write(int fd, char *ptr, int len)
{
  if(ptr)
  {
    char c = ptr[len];
    ptr[len] = 0;
    F_NOTICE("write(" << fd << ", " << ptr << ", " << len << ")");
    ptr[len] = c;
  }
  else
    ;
    F_NOTICE("write(" << fd << ", " << reinterpret_cast<uintptr_t>(ptr) << ", " << len << ")");

  // Lookup this process.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }
  FdMap &fdMap = pSubsystem->getFdMap();

  FileDescriptor *pFd = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fd));
  if (!pFd)
  {
    // Error - no such file descriptor.
    SYSCALL_ERROR(BadFileDescriptor);
    return -1;
  }

  // Copy to kernel.
  uint64_t nWritten = 0;
  if(ptr && len)
  {
    char *kernelBuf = new char[len];
    memcpy(reinterpret_cast<void*>(kernelBuf), reinterpret_cast<void*>(ptr), len);
    nWritten = pFd->file->write(pFd->offset, len, reinterpret_cast<uintptr_t>(kernelBuf));
    delete [] kernelBuf;
    pFd->offset += nWritten;
  }

  return static_cast<int>(nWritten);
}

int posix_lseek(int file, int ptr, int dir)
{
  F_NOTICE("lseek(" << file << ", " << ptr << ", " << dir << ")");

  // Lookup this process.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }
  FdMap &fdMap = pSubsystem->getFdMap();

  FileDescriptor *pFd = reinterpret_cast<FileDescriptor*>(fdMap.lookup(file));
  if (!pFd)
  {
    // Error - no such file descriptor.
    SYSCALL_ERROR(BadFileDescriptor);
    return -1;
  }

  size_t fileSize = pFd->file->getSize();
  switch (dir)
  {
    case SEEK_SET:
      pFd->offset = ptr;
      break;
    case SEEK_CUR:
      pFd->offset += ptr;
      break;
    case SEEK_END:
      pFd->offset = fileSize + ptr;
      break;
  }

  return static_cast<int>(pFd->offset);
}

int posix_link(char *old, char *_new)
{
  /// \note To make nethack work, you either have to implement this, or return 0 and pretend
  ///       it worked (ie, make the files in the tree - which I've already done -- Matt)
  NOTICE("posix_link(" << old << ", " << _new << ")");
  SYSCALL_ERROR(Unimplemented);
  return 0;
}

int posix_readlink(const char* path, char* buf, unsigned int bufsize)
{
  F_NOTICE("readlink(" << path << ", " << reinterpret_cast<uintptr_t>(buf) << ", " << bufsize << ")");

  File* f = VFS::instance().find(String(path), GET_CWD());
  if(!f)
  {
    SYSCALL_ERROR(DoesNotExist);
    return -1;
  }

  if(!f->isSymlink())
  {
    SYSCALL_ERROR(InvalidArgument);
    return -1;
  }

  if(buf == 0)
    return -1;

  HugeStaticString str;
  HugeStaticString tmp;
  str.clear();
  tmp.clear();

  return Symlink::fromFile(f)->followLink(buf, bufsize);
}

int posix_unlink(char *name)
{
  F_NOTICE("unlink(" << name << ")");

  if(VFS::instance().remove(String(name), GET_CWD()))
    return 0;
  else
    return -1; /// \todo SYSCALL_ERROR of some sort
}

int posix_rename(const char* source, const char* dst)
{
  F_NOTICE("rename(" << source << ", " << dst << ")");

  File* src = VFS::instance().find(String(source), GET_CWD());
  File* dest = VFS::instance().find(String(dst), GET_CWD());

  if (!src)
  {
    SYSCALL_ERROR(DoesNotExist);
    return -1;
  }

  // traverse symlink
  while(src->isSymlink())
    src = Symlink::fromFile(src)->followLink();

  if(dest)
  {
    // traverse symlink
    while(dest->isSymlink())
      dest = Symlink::fromFile(dest)->followLink();

    if(dest->isDirectory() && !src->isDirectory())
    {
      SYSCALL_ERROR(FileExists);
      return -1;
    }
    else if(!dest->isDirectory() && src->isDirectory())
    {
      SYSCALL_ERROR(NotADirectory);
      return -1;
    }
  }
  else
  {
    VFS::instance().createFile(String(dst), 0777, GET_CWD());
    dest = VFS::instance().find(String(dst), GET_CWD());
    if(!dest)
      return -1;
  }

  // Gay algorithm.
  uint8_t* buf = new uint8_t[src->getSize()];
  src->read(0, src->getSize(), reinterpret_cast<uintptr_t>(buf));
  dest->truncate();
  dest->write(0, src->getSize(), reinterpret_cast<uintptr_t>(buf));
  VFS::instance().remove(String(source), GET_CWD());

  return 0;
}

char* posix_getcwd(char* buf, size_t maxlen)
{
  F_NOTICE("getcwd(" << maxlen << ")");

  if(buf == 0)
    return 0;

  HugeStaticString str;
  HugeStaticString tmp;
  str.clear();
  tmp.clear();

  File* curr = GET_CWD();
  if(curr->getParent() != 0)
    str = curr->getName();

  File* f = curr;
  while((f = f->getParent()))
  {
    // This feels a bit weird considering the while loop's subject...
    if(f->getParent())
    {
      tmp = str;
      str = f->getName();
      str += "/";
      str += tmp;
    }
  }

  tmp = str;
  str = "/";
  str += tmp;

  strcpy(buf, static_cast<const char*>(str));

  return buf;
}

int posix_stat(const char *name, struct stat *st)
{
  F_NOTICE("stat(" << name << ")");

  // verify the filename - don't try to open a dud file (otherwise we'll open the cwd)
  if(name[0] == 0)
  {
    SYSCALL_ERROR(DoesNotExist);
    return -1;
  }

  File* file = VFS::instance().find(String(name), GET_CWD());

  if (!file)
  {
    SYSCALL_ERROR(DoesNotExist);
    return -1;
  }

  while (file->isSymlink())
    file = Symlink::fromFile(file)->followLink();

  if (!file)
  {
    // Error - not found.
    SYSCALL_ERROR(DoesNotExist);
    return -1;
  }

  int mode = 0;
  if (ConsoleManager::instance().isConsole(file))
  {
    mode = S_IFCHR;
  }
  else if (file->isDirectory())
  {
    mode = S_IFDIR;
  }
  else
  {
    mode = S_IFREG;
  }

  uint32_t permissions = file->getPermissions();
  if (permissions & FILE_UR) mode |= S_IRUSR;
  if (permissions & FILE_UW) mode |= S_IWUSR;
  if (permissions & FILE_UX) mode |= S_IXUSR;
  if (permissions & FILE_GR) mode |= S_IRGRP;
  if (permissions & FILE_GW) mode |= S_IWGRP;
  if (permissions & FILE_GX) mode |= S_IXGRP;
  if (permissions & FILE_OR) mode |= S_IROTH;
  if (permissions & FILE_OW) mode |= S_IWOTH;
  if (permissions & FILE_OX) mode |= S_IXOTH;

  st->st_dev   = static_cast<short>(reinterpret_cast<uintptr_t>(file->getFilesystem()));
  st->st_ino   = static_cast<short>(file->getInode());
  st->st_mode  = mode;
  st->st_nlink = 1;
  st->st_uid   = file->getUid();
  st->st_gid   = file->getGid();
  st->st_rdev  = 0;
  st->st_size  = static_cast<int>(file->getSize());
  st->st_atime = static_cast<int>(file->getAccessedTime());
  st->st_mtime = static_cast<int>(file->getModifiedTime());
  st->st_ctime = static_cast<int>(file->getCreationTime());

  return 0;
}

int posix_fstat(int fd, struct stat *st)
{
    F_NOTICE("fstat(" << fd << ")");
  // Lookup this process.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }
  FdMap &fdMap = pSubsystem->getFdMap();

  FileDescriptor *pFd = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fd));
  if (!pFd)
  {
    // Error - no such file descriptor.
    return -1;
  }

  int mode = 0;
  if (ConsoleManager::instance().isConsole(pFd->file))
  {
    mode = S_IFCHR;
  }
  else if (pFd->file->isDirectory())
  {
    mode = S_IFDIR;
  }
  else
  {
    mode = S_IFREG;
  }

  uint32_t permissions = pFd->file->getPermissions();
  if (permissions & FILE_UR) mode |= S_IRUSR;
  if (permissions & FILE_UW) mode |= S_IWUSR;
  if (permissions & FILE_UX) mode |= S_IXUSR;
  if (permissions & FILE_GR) mode |= S_IRGRP;
  if (permissions & FILE_GW) mode |= S_IWGRP;
  if (permissions & FILE_GX) mode |= S_IXGRP;
  if (permissions & FILE_OR) mode |= S_IROTH;
  if (permissions & FILE_OW) mode |= S_IWOTH;
  if (permissions & FILE_OX) mode |= S_IXOTH;

  st->st_dev   = static_cast<short>(reinterpret_cast<uintptr_t>(pFd->file->getFilesystem()));
  st->st_ino   = static_cast<short>(pFd->file->getInode());
  st->st_mode  = mode;
  st->st_nlink = 1;
  st->st_uid   = pFd->file->getUid();
  st->st_gid   = pFd->file->getGid();
  st->st_rdev  = 0;
  st->st_size  = static_cast<int>(pFd->file->getSize());
  st->st_atime = static_cast<int>(pFd->file->getAccessedTime());
  st->st_mtime = static_cast<int>(pFd->file->getModifiedTime());
  st->st_ctime = static_cast<int>(pFd->file->getCreationTime());
  return 0;
}

int posix_lstat(char *name, struct stat *st)
{
  F_NOTICE("lstat(" << name << ")");

  File *file = VFS::instance().find(String(name), GET_CWD());

  int mode = 0;
  if (!file)
  {
    // Error - not found.
    SYSCALL_ERROR(DoesNotExist);
    return -1;
  }
  if (file->isSymlink())
  {
    mode = S_IFLNK;
  }
  else
  {
    if (ConsoleManager::instance().isConsole(file))
    {
      mode = S_IFCHR;
    }
    else if (file->isDirectory())
    {
      mode = S_IFDIR;
    }
    else
    {
      mode = S_IFREG;
    }
  }

  uint32_t permissions = file->getPermissions();
  if (permissions & FILE_UR) mode |= S_IRUSR;
  if (permissions & FILE_UW) mode |= S_IWUSR;
  if (permissions & FILE_UX) mode |= S_IXUSR;
  if (permissions & FILE_GR) mode |= S_IRGRP;
  if (permissions & FILE_GW) mode |= S_IWGRP;
  if (permissions & FILE_GX) mode |= S_IXGRP;
  if (permissions & FILE_OR) mode |= S_IROTH;
  if (permissions & FILE_OW) mode |= S_IWOTH;
  if (permissions & FILE_OX) mode |= S_IXOTH;

  st->st_dev   = static_cast<short>(reinterpret_cast<uintptr_t>(file->getFilesystem()));
  st->st_ino   = static_cast<short>(file->getInode());
  st->st_mode  = mode;
  st->st_nlink = 1;
  st->st_uid   = file->getGid();
  st->st_gid   = file->getGid();
  st->st_rdev  = 0;
  st->st_size  = static_cast<int>(file->getSize());
  st->st_atime = static_cast<int>(file->getAccessedTime());
  st->st_mtime = static_cast<int>(file->getModifiedTime());
  st->st_ctime = static_cast<int>(file->getCreationTime());

  return 0;
}

int posix_opendir(const char *dir, dirent *ent)
{
  F_NOTICE("opendir(" << dir << ")");

  // Lookup this process.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }
  FdMap &fdMap = pSubsystem->getFdMap();

  size_t fd = pSubsystem->nextFd();

  File* file = VFS::instance().find(String(dir), GET_CWD());

  if (!file)
  {
    // Error - not found.
    SYSCALL_ERROR(DoesNotExist);
    return -1;
  }

  while (file->isSymlink())
    file = Symlink::fromFile(file)->followLink();

  if (!file->isDirectory())
  {
    // Error - not a directory.
    SYSCALL_ERROR(NotADirectory);
    return -1;
  }

  FileDescriptor *f = new FileDescriptor;
  f->file = file;
  f->offset = 0;
  f->fd = fd;


  file = Directory::fromFile(file)->getChild(0);
  if(file)
    ent->d_ino = file->getInode();
  else
  {
    // fail! no children!
    delete f;
    return -1;
  }

  fdMap.insert(fd, f);

  return static_cast<int>(fd);
}

int posix_readdir(int fd, dirent *ent)
{
  F_NOTICE("readdir(" << fd << ")");

  if(fd == -1)
    return -1;

  // Lookup this process.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }
  FdMap &fdMap = pSubsystem->getFdMap();

  FileDescriptor *pFd = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fd));
  if (!pFd)
  {
    // Error - no such file descriptor.
    return -1;
  }

  /// \todo Sanity checks.
  File* file = Directory::fromFile(pFd->file)->getChild(pFd->offset);
  if (!file)
    return -1;

  ent->d_ino = static_cast<short>(file->getInode());
  String tmp = file->getName();
  strcpy(ent->d_name, static_cast<const char*>(tmp));
  ent->d_name[strlen(static_cast<const char*>(tmp))] = '\0';
  pFd->offset ++;

  return 0;
}

void posix_rewinddir(int fd, dirent *ent)
{
  if(fd == -1)
    return;

  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return;
  }
  FileDescriptor *f = reinterpret_cast<FileDescriptor*>(pSubsystem->getFdMap().lookup(fd));
  f->offset = 0;
  posix_readdir(fd, ent);
}

int posix_closedir(int fd)
{
  if(fd == -1)
    return -1;

  /// \todo Race here - fix.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }
  FdMap &fdMap = pSubsystem->getFdMap();
  FileDescriptor *f = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fd));
  fdMap.remove(fd);

  delete f;
  return 0;
}

int posix_ioctl(int fd, int command, void *buf)
{
  F_NOTICE("ioctl(" << Dec << fd << ", " << Hex << command << ", " << reinterpret_cast<uintptr_t>(buf) << ")");

  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }
  FileDescriptor *f = reinterpret_cast<FileDescriptor*>(pSubsystem->getFdMap().lookup(fd));

  if (!f)
  {
    return -1;
    // Error - no such FD.
  }

  switch (command)
  {
    case TIOCGWINSZ:
    {
      return console_getwinsize(f->file, reinterpret_cast<winsize_t*>(buf));
    }
    case FIONBIO:
    {
      // set/unset non-blocking
      if(buf)
      {
        int a = *reinterpret_cast<int *>(buf);
        NOTICE("a = " << a << ".");
        if(a)
          f->flflags = O_NONBLOCK;
        else
          f->flflags ^= O_NONBLOCK;
      }
      else
        f->flflags = 0;

      return 0;
    }
    default:
    {
      // Error - no such ioctl.
      return -1;
    }
  }
}

int posix_chdir(const char *path)
{
  F_NOTICE("chdir(" << path << ")");

  File *dir = VFS::instance().find(String(path), GET_CWD());
  if (dir)
  {
    Processor::information().getCurrentThread()->getParent()->setCwd(dir);

    char tmp[1024];
    posix_getcwd(tmp, 1024);
    return 0;
  }
  else
  {
    SYSCALL_ERROR(DoesNotExist);
    return -1;
  }

  return 0;
}

int posix_dup(int fd)
{
  F_NOTICE("dup(" << fd << ")");

  // grab the file descriptor pointer for the passed descriptor
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }
  FdMap &fdMap = pSubsystem->getFdMap();
  FileDescriptor *f = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fd));

  if(!f)
  {
    SYSCALL_ERROR(BadFileDescriptor);
    return -1;
  }

  size_t newFd = pSubsystem->nextFd();

  // copy the descriptor
  FileDescriptor* f2 = new FileDescriptor;
  f2->file = f->file;
  f2->offset = f->offset;

  f2->fdflags = f->fdflags;
  f2->flflags = f->flflags;

  fdMap.insert(newFd, f2);

  f2->file->increaseRefCount((f2->flflags & O_RDWR) || (f2->flflags & O_WRONLY) );

  return static_cast<int>(newFd);
}

int posix_dup2(int fd1, int fd2)
{
  F_NOTICE("dup2(" << fd1 << ", " << fd2 << ")");

  if(fd2 < 0)
  {
    SYSCALL_ERROR(BadFileDescriptor);
    return -1; // EBADF
  }

  if(fd1 == fd2)
    return fd2;

  // grab the file descriptor pointer for the passed descriptor
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }
  FdMap &fdMap = pSubsystem->getFdMap();
  FileDescriptor* f = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fd1));

  if(!f)
  {
    SYSCALL_ERROR(BadFileDescriptor);
    return -1;
  }

  // Increase the refcount *before* we close the original, else we might
  // accidentally trigger an EOF condition on a pipe! (if the write refcount drops
  // to zero)
  f->file->increaseRefCount((f->flflags & O_RDWR) || (f->flflags & O_WRONLY) );

  // close the original descriptor
  if(posix_close(fd2) == -1)
    return -1;

  // copy the descriptor
  FileDescriptor* f2 = new FileDescriptor;
  f2->file = f->file;
  f2->offset = f->offset;

  f2->fdflags = f->fdflags;
  f2->flflags = f->flflags;

  fdMap.insert(fd2, f2);

  return static_cast<int>(fd2);
}

int posix_mkdir(const char* name, int mode)
{
  F_NOTICE("mkdir(" << name << ")");
  bool worked = VFS::instance().createDirectory(String(name), GET_CWD());
  return worked ? 0 : -1;
}

int posix_isatty(int fd)
{
  // Lookup this process.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }
  FdMap &fdMap = pSubsystem->getFdMap();

  FileDescriptor *pFd = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fd));
  if (!pFd)
  {
      // Error - no such file descriptor.
      ERROR("isatty: no such file descriptor (" << Dec << fd << ")");
      return 0;
  }

  return (ConsoleManager::instance().isConsole(pFd->file)) ? 1 : 0;
}

class SelectTask : public TimedTask
{
    public:
        SelectTask() :
            TimedTask(), pFile(0), bWriting(false), fd(-1)
        {};
        virtual ~SelectTask()
        {};

        virtual int task()
        {
            // timeout is handled by *this* object, not by the File::select function
            return pFile->select(bWriting, m_Timeout);
        }

        File *pFile;
        bool bWriting;

        int fd;
    private:

        SelectTask(const SelectTask&);
        const SelectTask& operator = (const SelectTask&);
};

int posix_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, timeval *timeout)
{
    F_NOTICE("select(" << Dec << nfds << Hex << ")");

    // Lookup this process.
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if(!pSubsystem)
    {
      ERROR("No subsystem for this process!");
      return -1;
    }
    FdMap &fdMap = pSubsystem->getFdMap();

    List<SelectTask *> tasks;
    List<Semaphore *> taskSems;

    uint32_t timeoutSeconds = 0;
    if(timeout)
      timeoutSeconds = timeout->tv_sec;

    int num_ready = 0;
    for (int i = 0; i < nfds; i++)
    {
        // valid fd?
        FileDescriptor *pFd = 0;
        if((readfds && FD_ISSET(i, readfds)) || (writefds && FD_ISSET(i, writefds)))
        {
          pFd = reinterpret_cast<FileDescriptor*>(fdMap.lookup(i));
          if (!pFd)
          {
              // Error - no such file descriptor.
              ERROR("select: no such file descriptor (" << Dec << i << ")");
              return -1;
          }
        }

        if(readfds)
        {
            if (FD_ISSET(i, readfds))
            {
                /// \todo Subclass File for consoles, then this whole thing can be simplified further!
                if (ConsoleManager::instance().isConsole(pFd->file))
                {
                    if (ConsoleManager::instance().hasDataAvailable(pFd->file))
                        num_ready++;
                    else
                        FD_CLR(i, readfds);

                    continue;
                }

                SelectTask *t = new SelectTask;
                t->pFile = pFd->file;
                t->bWriting = false;
                t->fd = i;
                t->setTimeout(timeoutSeconds);

                Semaphore *s = new Semaphore(0);
                t->setSemaphore(s);

                tasks.pushBack(t);
                taskSems.pushBack(s);

                t->begin();
            }
        }
        if(writefds)
        {
            if (FD_ISSET(i, writefds))
            {
                SelectTask *t = new SelectTask;
                t->pFile = pFd->file;
                t->bWriting = true;
                t->fd = i;
                t->setTimeout(timeoutSeconds);

                Semaphore *s = new Semaphore(0);
                t->setSemaphore(s);

                tasks.pushBack(t);
                taskSems.pushBack(s);

                t->begin();
            }
        }
    }

    // We've probably got a bunch of tasks now, and a bunch of Semaphores to check for completion of each one.
    // Let's check those tasks!
    if(tasks.count())
    {
        while(true)
        {
            bool bOneHasCompleted = false;
            List<SelectTask *>::Iterator it = tasks.begin();
            List<Semaphore *>::Iterator it2 = taskSems.begin();
            for(; it != tasks.end() && it2 != taskSems.end(); it++, it2++)
            {
                // check the semaphore for completion
                if((*it2)->tryAcquire())
                {
                    if((*it)->wasSuccessful())
                    {
                        int ret = (*it)->getReturnValue();
                        if(ret == 0)
                            FD_CLR((*it)->fd, (*it)->bWriting ? writefds : readfds);
                        num_ready += ret;
                    }
                    else // timed out or cancelled elsewhere!
                        FD_CLR((*it)->fd, (*it)->bWriting ? writefds : readfds);

                    bOneHasCompleted = true;

                    // clean up - now that one has completed the function will be returning
                    delete (*it);
                    delete (*it2);
                }
                else if(bOneHasCompleted)
                {
                    // one's already done, this one isn't complete - cancel it & clean up
                    (*it)->cancel();
                    delete (*it);
                    delete (*it2);
                }
            }

            if(bOneHasCompleted)
                break;

            Scheduler::instance().yield();
        }
    }

    // Kill zombie threads created by TimedTask
    for(size_t i = 0; i < pProcess->getNumThreads(); i++)
    {
        Thread *pThread = pProcess->getThread(i);
        if(pThread->getStatus() == Thread::Zombie)
          delete pThread;
    }

    return num_ready;
}

int posix_fcntl(int fd, int cmd, int num, int* args)
{
  if(num)
    F_NOTICE("fcntl(" << fd << ", " << cmd << ", " << num << ", " << args[0] << ")");
  else
    F_NOTICE("fcntl(" << fd << ", " << cmd << ")");

  // grab the file descriptor pointer for the passed descriptor
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }
  FdMap &fdMap = pSubsystem->getFdMap();
  FileDescriptor* f = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fd));
  if(!f)
  {
    SYSCALL_ERROR(BadFileDescriptor);
    return -1;
  }

  switch(cmd)
  {
    case F_DUPFD:

      if(num)
      {
        // if there is an argument and it's valid, map fd to the passed descriptor
        if(args[0] >= 0)
        {
          size_t fd2 = static_cast<size_t>(args[0]);

          // Lookup this process.
          FileDescriptor* f1 = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fd2));
          if(f1)
          {
            fdMap.remove(fd2);
            f1->file->decreaseRefCount((f1->flflags & O_RDWR) || (f1->flflags & O_WRONLY) );    NOTICE("lkg");
            delete f1;
          }

          // copy the descriptor
          FileDescriptor* f2 = new FileDescriptor;
          f2->file = f->file;
          f2->offset = f->offset;
          f2->fdflags = f->fdflags;
          f2->flflags = f->flflags;
          fdMap.insert(fd2, f2);
          f2->file->increaseRefCount((f2->flflags & O_RDWR) || (f2->flflags & O_WRONLY) );

          return static_cast<int>(fd2);
        }
      }
      else
      {
        size_t fd2 = pSubsystem->nextFd();

        // copy the descriptor
        FileDescriptor* f2 = new FileDescriptor;
        f2->file = f->file;
        f2->offset = f->offset;
        f2->fdflags = f->fdflags;
        f2->flflags = f->flflags;
        fdMap.insert(fd2, f2);

        f2->file->increaseRefCount((f2->flflags & O_RDWR) || (f2->flflags & O_WRONLY) );

        return static_cast<int>(fd2);
      }
    case F_GETFD:
      return f->fdflags;
    case F_SETFD:
      f->fdflags = args[0];
      return 0;
    case F_GETFL:
      return f->flflags;
    case F_SETFL:
      f->flflags = args[0];
      return 0;
  }

  SYSCALL_ERROR(Unimplemented);
  return -1;
}

int posix_poll(struct pollfd* fds, unsigned int nfds, int timeout)
{
  NOTICE("poll(" << Dec << nfds << Hex << ")");

  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for this process!");
      return -1;
  }
  FdMap &fdMap = pSubsystem->getFdMap();

  unsigned int i;
  int num_ready = 0;
  for(i = 0; i < nfds; i++)
  {
    FileDescriptor *pFd = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fds[i].fd));

    // readable?
    if(fds[i].events & POLLIN)
    {
      if (ConsoleManager::instance().isConsole(pFd->file))
      {
        //NOTICE("is a console");
        if (ConsoleManager::instance().hasDataAvailable(pFd->file))
          num_ready++;
      }
      else if (NetManager::instance().isEndpoint(pFd->file))
      {
        Endpoint* p = NetManager::instance().getEndpoint(pFd->file);
        if(!p)
        {
          continue;
        }

        if(timeout)
        {
          if(p->dataReady(true, timeout))
            num_ready++;
          else
            fds[i].revents = 0;
        }
        else
        {
          // don't wait for data
          if(p->dataReady(false))
            num_ready++;
          else
            fds[i].revents = 0;
        }
      }
      else
        // Regular file - always available to read.
        num_ready++;
    }

    // writeable?
    if(fds[i].events & POLLOUT)
    {
      if (NetManager::instance().isEndpoint(pFd->file))
      {
        Endpoint* p = NetManager::instance().getEndpoint(pFd->file);
        if(!p)
          continue;

        int state = p->state();

        if(state == 0xff)
          num_ready++;
        else
        {
          // writeable state? (though technically FIN_WAIT_2 isn't writeable)
          /// \todo Timeout lets this check block, but this code won't block...
          if(state >= Tcp::ESTABLISHED && state < Tcp::CLOSE_WAIT)
            num_ready++;
        }
      }
      else
        num_ready++; // normal files are always ready to write
    }
  }

  return num_ready;
}

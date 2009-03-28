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
#include <vfs/VFS.h>
#include <console/Console.h>
#include <network/NetManager.h>
#include <utilities/utility.h>

#include "file-syscalls.h"
#include "console-syscalls.h"
#include "pipe-syscalls.h"

#define __IOCTL_FIRST 0x1000

#define TIOCGWINSZ  0x1000  /* Get console window size. */
#define TIOCSWINSZ  0x1001  /* Set console window size. */

#define __IOCTL_LAST  0x1001

// From newlib's stdio.h
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// From newlib's fcntl.h
#define O_RDONLY 0x000
#define O_WRONLY 0x001
#define O_RDWR   0x002
#define O_APPEND 0x008
#define O_CREAT  0x200
#define O_TRUNC  0x400
#define O_EXCL   0x800 // Error on open if file exists.

extern int posix_getpid();

NullFs NullFs::m_Instance;

//
// Syscalls pertaining to files.
//

typedef Tree<size_t,FileDescriptor*> FdMap;

#define GET_CWD() (Processor::information().getCurrentThread()->getParent()->getCwd())

int posix_close(int fd)
{
  NOTICE("close(" << fd << ")");
  /// \todo Race here - fix.
  FileDescriptor *f = reinterpret_cast<FileDescriptor*>(Processor::information().getCurrentThread()->getParent()->getFdMap().lookup(fd));

  if (!f)
  {
    SYSCALL_ERROR(BadFileDescriptor);
    return -1;
  }

  if(NetManager::instance().isEndpoint(f->file))
  {
    NOTICE("Closing the endpoint...");
    NetManager::instance().removeEndpoint(f->file);
  }

  Processor::information().getCurrentThread()->getParent()->getFdMap().remove(fd);
  if (f->file->shouldDelete()) delete f->file;
  delete f;
  return 0;
}

int posix_open(const char *name, int flags, int mode)
{
  NOTICE("open(" << name << ")");

  // verify the filename - don't try to open a dud file
  if(name[0] == 0)
  {
    SYSCALL_ERROR(DoesNotExist);
    return -1;
  }
  
  // Lookup this process.
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

  size_t fd = Processor::information().getCurrentThread()->getParent()->nextFd();

  // Check for /dev/tty, and link to our controlling console.
  File* file = VFS::invalidFile();

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
  if (!file->isValid())
  {
    if(flags & O_CREAT)
    {
      bool worked = VFS::instance().createFile(String(name), 0777, GET_CWD());
      if(!worked)
      {
        SYSCALL_ERROR(DoesNotExist);
        return -1;
      }

      file = VFS::instance().find(String(name), GET_CWD());
      if (!file->isValid())
      {
        SYSCALL_ERROR(DoesNotExist);
        return -1;
      }

      bCreated = true;
    }
    else
    {
      // Error - not found.
      SYSCALL_ERROR(DoesNotExist);
      return -1;
    }
  }

  while (file->isSymlink())
    file = file->followLink();

  if (file->isDirectory())
  {
    // Error - is directory.
    SYSCALL_ERROR(IsADirectory);
    return -1;
  }

  if((flags & O_CREAT) && (flags & O_EXCL) && !bCreated)
  {
    // file exists with O_CREAT and O_EXCL
    SYSCALL_ERROR(FileExists);
    return -1;
  }

  if((flags & O_TRUNC) && ((flags & O_CREAT) || (flags & O_WRONLY) || (flags & O_RDWR)))
  {
    NOTICE("Truncating file");
    
    // truncate the file
    file->truncate();
  }
  
  FileDescriptor *f = new FileDescriptor;
  f->file = file;
  f->offset = (flags & O_APPEND) ? file->getSize() : 0;
  f->fd = fd;
  fdMap.insert(fd, f);

  return static_cast<int> (fd);
}

int posix_read(int fd, char *ptr, int len)
{
  NOTICE("read(" << Dec << fd << ", " << Hex << reinterpret_cast<uintptr_t>(ptr) << ", " << len << ")");

  // Lookup this process.
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

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
    //    NOTICE("write(" << fd << ", " << ptr << ", " << len << ")");
    ptr[len] = c;
  }
  else
    NOTICE("write(" << fd << ", " << reinterpret_cast<uintptr_t>(ptr) << ", " << len << ")");

  // Lookup this process.
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

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
  NOTICE("lseek(" << file << ", " << ptr << ", " << dir << ")");

  // Lookup this process.
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

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
  SYSCALL_ERROR(Unimplemented);
  return -1;
}

int posix_unlink(char *name)
{
  NOTICE("unlink(" << name << ")");

  if(VFS::instance().remove(String(name), GET_CWD()))
    return 0;
  else
    return -1; /// \todo SYSCALL_ERROR of some sort
}

int posix_stat(const char *name, struct stat *st)
{
  NOTICE("stat(" << name << ")");

  // verify the filename - don't try to open a dud file (otherwise we'll open the cwd)
  if(name[0] == 0)
  {
    SYSCALL_ERROR(DoesNotExist);
    return -1;
  }

  File* file = VFS::instance().find(String(name), GET_CWD());

  while (file->isSymlink())
    file = file->followLink();

  if (!file->isValid())
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

  st->st_dev   = static_cast<short>(reinterpret_cast<uintptr_t>(file->getFilesystem()));
  st->st_ino   = static_cast<short>(file->getInode());
  st->st_mode  = mode;
  st->st_nlink = 1;
  st->st_uid   = 0;
  st->st_gid   = 0;
  st->st_rdev  = 0;
  st->st_size  = static_cast<int>(file->getSize());
  st->st_atime = static_cast<int>(file->getAccessedTime());
  st->st_mtime = static_cast<int>(file->getModifiedTime());
  st->st_ctime = static_cast<int>(file->getCreationTime());

  return 0;
}

int posix_fstat(int fd, struct stat *st)
{
  // Lookup this process.
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

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

  st->st_dev   = static_cast<short>(reinterpret_cast<uintptr_t>(pFd->file->getFilesystem()));
  st->st_ino   = static_cast<short>(pFd->file->getInode());
  st->st_mode  = mode;
  st->st_nlink = 1;
  st->st_uid   = 0;
  st->st_gid   = 0;
  st->st_rdev  = 0;
  st->st_size  = static_cast<int>(pFd->file->getSize());
  st->st_atime = static_cast<int>(pFd->file->getAccessedTime());
  st->st_mtime = static_cast<int>(pFd->file->getModifiedTime());
  st->st_ctime = static_cast<int>(pFd->file->getCreationTime());
  return 0;
}

int posix_lstat(char *name, struct stat *st)
{
  NOTICE("lstat(" << name << ")");

  File *file = VFS::instance().find(String(name), GET_CWD());

  int mode = 0;
  if (!file->isValid())
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

  st->st_dev   = static_cast<short>(reinterpret_cast<uintptr_t>(file->getFilesystem()));
  st->st_ino   = static_cast<short>(file->getInode());
  st->st_mode  = mode;
  st->st_nlink = 1;
  st->st_uid   = 0;
  st->st_gid   = 0;
  st->st_rdev  = 0;
  st->st_size  = static_cast<int>(file->getSize());
  st->st_atime = static_cast<int>(file->getAccessedTime());
  st->st_mtime = static_cast<int>(file->getModifiedTime());
  st->st_ctime = static_cast<int>(file->getCreationTime());

  return 0;
}

int posix_opendir(const char *dir, dirent *ent)
{
  NOTICE("opendir(" << dir << ")");

  // Lookup this process.
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

  size_t fd = Processor::information().getCurrentThread()->getParent()->nextFd();

  File* file = VFS::instance().find(String(dir), GET_CWD());

  if (!file->isValid())
  {
    // Error - not found.
    SYSCALL_ERROR(DoesNotExist);
    return -1;
  }

  while (file->isSymlink())
    file = file->followLink();

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
  fdMap.insert(fd, f);


  file = file->getChild(0);
  ent->d_ino = file->getInode();

  return static_cast<int>(fd);
}

int posix_readdir(int fd, dirent *ent)
{
  NOTICE("readdir(" << fd << ")");
  
  if(fd == -1)
    return -1;
  
  // Lookup this process.
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

  FileDescriptor *pFd = reinterpret_cast<FileDescriptor*>(fdMap.lookup(fd));
  if (!pFd)
  {
    // Error - no such file descriptor.
    return -1;
  }

  /// \todo Sanity checks.
  File* file = pFd->file->getChild(pFd->offset);
  if (!file->isValid())
    return -1;

  ent->d_ino = static_cast<short>(file->getInode());
  String tmp = file->getName();
  strcpy(ent->d_name, static_cast<const char*>(tmp));
  ent->d_name[strlen(static_cast<const char*>(tmp))] = '\0';
  NOTICE("tmp: " << (uintptr_t)pFd->file << ", " << pFd->file->getName());
  pFd->offset ++;

  return 0;
}

void posix_rewinddir(int fd, dirent *ent)
{
  if(fd == -1)
    return;
  
  FileDescriptor *f = reinterpret_cast<FileDescriptor*>(Processor::information().getCurrentThread()->getParent()->getFdMap().lookup(fd));
  f->offset = 0;
  posix_readdir(fd, ent);
}

int posix_closedir(int fd)
{
  if(fd == -1)
    return -1;
  
  /// \todo Race here - fix.
  FileDescriptor *f = reinterpret_cast<FileDescriptor*>(Processor::information().getCurrentThread()->getParent()->getFdMap().lookup(fd));
  Processor::information().getCurrentThread()->getParent()->getFdMap().remove(fd);
  if (f->file->shouldDelete()) delete f->file;
  delete f;
  return 0;
}

int posix_ioctl(int fd, int command, void *buf)
{
  NOTICE("ioctl(" << Dec << fd << ", " << Hex << command << ")");
  FileDescriptor *f = reinterpret_cast<FileDescriptor*>(Processor::information().getCurrentThread()->getParent()->getFdMap().lookup(fd));
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
    default:
    {
      // Error - no such ioctl.
      return -1;
    }
  }
}

int posix_chdir(const char *path)
{
  NOTICE("chdir(" << path << ")");

  File *dir = VFS::instance().find(String(path), GET_CWD());
  if (dir && dir->isValid())
  {
    Processor::information().getCurrentThread()->getParent()->setCwd(dir);
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
  NOTICE("dup(" << fd << ")");
  
  // grab the file descriptor pointer for the passed descriptor
  FileDescriptor *f = reinterpret_cast<FileDescriptor*>(Processor::information().getCurrentThread()->getParent()->getFdMap().lookup(fd));
  if(!f)
  {
    /// \todo SYSCALL_ERROR of some form
    SYSCALL_ERROR(BadFileDescriptor);
    return -1;
  }

  // Lookup this process.
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

  size_t newFd = Processor::information().getCurrentThread()->getParent()->nextFd();

  // copy the descriptor
  FileDescriptor* f2 = new FileDescriptor;
  f2->file = PipeManager::instance().copyPipe(f->file);
  f2->offset = f->offset;

  f2->fdflags = f->fdflags;
  f2->flflags = f->flflags;

  fdMap.insert(newFd, f2);

  return static_cast<int>(newFd);
}

int posix_dup2(int fd1, int fd2)
{
  NOTICE("dup2(" << fd1 << ", " << fd2 << ")");

  if(fd2 < 0) /// \todo OR fd2 >= OPEN_MAX
  {
    SYSCALL_ERROR(BadFileDescriptor);
    return -1; // EBADF
  }

  if(fd1 == fd2)
    return fd2;

  // grab the file descriptor pointer for the passed descriptor
  FileDescriptor* f = reinterpret_cast<FileDescriptor*>(Processor::information().getCurrentThread()->getParent()->getFdMap().lookup(fd1));
  if(!f)
  {
    SYSCALL_ERROR(BadFileDescriptor);
    return -1;
  }

  // close the original descriptor
  if(posix_close(fd2) == -1)
    return -1;

  // Lookup this process.
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

  // copy the descriptor
  FileDescriptor* f2 = new FileDescriptor;
  f2->file = PipeManager::instance().copyPipe(f->file);
  f2->offset = f->offset;

  f2->fdflags = f->fdflags;
  f2->flflags = f->flflags;

  fdMap.insert(fd2, f2);

  return static_cast<int>(fd2);
}

int posix_mkdir(const char* name, int mode)
{
  bool worked = VFS::instance().createDirectory(String(name), GET_CWD());
  return worked ? 0 : -1;
}

int posix_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, timeval *timeout)
{
  /// \note This is by no means a full select() implementation! It only implements the functionality required for nano, which is not much.
  ///       Just readfds, and no timeout.
  NOTICE("select(" << Dec << nfds << Hex << ")");

  // Lookup this process.
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();

  int num_ready = 0;
  for (int i = 0; i < nfds; i++)
  {
    if (FD_ISSET(i, readfds))
    {
      // Is the FD a console?
      FileDescriptor *pFd = reinterpret_cast<FileDescriptor*>(fdMap.lookup(i));
      if (!pFd)
      {
        // Error - no such file descriptor.
        ERROR("select: no such file descriptor (" << Dec << i << ")");
        return -1;
      }

      if (ConsoleManager::instance().isConsole(pFd->file))
      {
        if (ConsoleManager::instance().hasDataAvailable(pFd->file))
          num_ready ++;
        else
          FD_CLR(i, readfds);
      }
      else if (NetManager::instance().isEndpoint(pFd->file))
      {
        Endpoint* p = NetManager::instance().getEndpoint(pFd->file);
        if(!p)
        {
          FD_CLR(i, readfds);
          continue;
        }
        
        if(timeout)
        {
          if(timeout->tv_sec == 0)
          {
            int val = p->recv(0, 0, false, 5); // 5 = MSG_PEEK
            if(val >= 0) /// \todo Zero means EOF, if recv isn't blocking and has no data it'll return zero...
              num_ready++;
            else
              NOTICE("Failed, val = " << Dec << val << Hex << ".");
          }
          else
          {
            p->dataReady(true, timeout->tv_sec);
            num_ready++;
          }
        }
        else
        {
          // block while waiting for the data to come (and wait forever)
          p->dataReady(true, 0xffffffff);
          num_ready++;
        }
      }
      else
        // Regular file - always available to read.
        num_ready ++;
    }
  }

  return num_ready;
}

int posix_fcntl(int fd, int cmd, int num, int* args)
{
  if(num)
    NOTICE("fcntl(" << fd << ", " << cmd << ", " << num << ", " << args[0] << ")");
  else
    NOTICE("fcntl(" << fd << ", " << cmd << ")");
  
  // grab the file descriptor pointer for the passed descriptor
  FdMap &fdMap = Processor::information().getCurrentThread()->getParent()->getFdMap();
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
            if (f1->file->shouldDelete()) delete f1->file;
            delete f1;
          }
          
          // copy the descriptor
          FileDescriptor* f2 = new FileDescriptor;
          f2->file = PipeManager::instance().copyPipe(f->file);
          f2->offset = f->offset;
          f2->fdflags = f->fdflags;
          f2->flflags = f->flflags;
          fdMap.insert(fd2, f2);
          
          return static_cast<int>(fd2);
        }
      }
      else
      {
        size_t fd2 = Processor::information().getCurrentThread()->getParent()->nextFd();
        
        // copy the descriptor
        FileDescriptor* f2 = new FileDescriptor;
        f2->file = PipeManager::instance().copyPipe(f->file);
        f2->offset = f->offset;
        f2->fdflags = f->fdflags;
        f2->flflags = f->flflags;
        fdMap.insert(fd2, f2);
        
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

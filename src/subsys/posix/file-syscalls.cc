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
#include <vfs/LockedFile.h>
#include <vfs/MemoryMappedFile.h>
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

#define GET_CWD() (Processor::information().getCurrentThread()->getParent()->getCwd())

int posix_close(int fd)
{
    F_NOTICE("close(" << fd << ")");
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    pSubsystem->freeFd(fd);
    return 0;
}

int posix_open(const char *name, int flags, int mode)
{
    // Verify that the filename is valid
    if(!name)
    {
      F_NOTICE("open called with null filename");
      SYSCALL_ERROR(DoesNotExist);
      return -1;
    }

    F_NOTICE("open(" << name << ", " << ((mode&O_RDWR)?"O_RDWR":"") << ((mode&O_RDONLY)?"O_RDONLY":"") << ((mode&O_WRONLY)?"O_WRONLY":"") << ")");

    // verify the filename - don't try to open a dud file
    if (name[0] == 0)
    {
        SYSCALL_ERROR(DoesNotExist);
        return -1;
    }

    // Lookup this process.
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    size_t fd = pSubsystem->getFd();

    // Check for /dev/tty, and link to our controlling console.
    File* file = 0;

    if (!strcmp(name, "/dev/tty"))
    {
        file = pProcess->getCtty();
        if (!file)
            file = NullFs::instance().getFile();
    }
    else if (!strcmp(name, "/dev/null"))
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
        if (flags & O_CREAT)
        {
            F_NOTICE("  {O_CREAT}");
            bool worked = VFS::instance().createFile(String(name), 0777, GET_CWD());
            if (!worked)
            {
                SYSCALL_ERROR(DoesNotExist);
                pSubsystem->freeFd(fd);
                return -1;
            }

            file = VFS::instance().find(String(name), GET_CWD());
            if (!file)
            {
                SYSCALL_ERROR(DoesNotExist);
                pSubsystem->freeFd(fd);
                return -1;
            }

            bCreated = true;
        }
        else
        {
            F_NOTICE("Does not exist.");
            // Error - not found.
            SYSCALL_ERROR(DoesNotExist);
            pSubsystem->freeFd(fd);
            return -1;
        }
    }

    while(file && file->isSymlink())
        file = Symlink::fromFile(file)->followLink();
    if(!file)
    {
      SYSCALL_ERROR(DoesNotExist);
      pSubsystem->freeFd(fd);
      return -1;
    }

    if (file->isDirectory())
    {
        // Error - is directory.
        F_NOTICE("Is a directory.");
        SYSCALL_ERROR(IsADirectory);
        pSubsystem->freeFd(fd);
        return -1;
    }

    if ((flags & O_CREAT) && (flags & O_EXCL) && !bCreated)
    {
        // file exists with O_CREAT and O_EXCL
        F_NOTICE("File exists");
        SYSCALL_ERROR(FileExists);
        pSubsystem->freeFd(fd);
        return -1;
    }

    if ((flags & O_TRUNC) && ((flags & O_CREAT) || (flags & O_WRONLY) || (flags & O_RDWR)))
    {
        F_NOTICE("  {O_TRUNC}");
        // truncate the file
        file->truncate();
    }

    if(file)
    {
      FileDescriptor *f = new FileDescriptor(file, (flags & O_APPEND) ? file->getSize() : 0, fd, 0, flags | mode);
      if(f)
        pSubsystem->addFileDescriptor(fd, f);
    }
    else
    {
      SYSCALL_ERROR(DoesNotExist);
      pSubsystem->freeFd(fd);
      return -1;
    }

    return static_cast<int> (fd);
}

int posix_read(int fd, char *ptr, int len)
{
    F_NOTICE("read(" << Dec << fd << Hex << ", " << reinterpret_cast<uintptr_t>(ptr) << ", " << len << ")");

    // Lookup this process.
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    FileDescriptor *pFd = pSubsystem->getFileDescriptor(fd);
    if (!pFd)
    {
        // Error - no such file descriptor.
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }

    // Are we allowed to block?
    bool canBlock = !((pFd->flflags & O_NONBLOCK) == O_NONBLOCK);

    uint64_t nRead = 0;
    if (ptr && len)
    {
        char *kernelBuf = new char[len];
        nRead = pFd->file->read(pFd->offset, len, reinterpret_cast<uintptr_t>(kernelBuf), canBlock);
        memcpy(reinterpret_cast<void*>(ptr), reinterpret_cast<void*>(kernelBuf), len);
        delete [] kernelBuf;

        pFd->offset += nRead;
    }

    return static_cast<int>(nRead);
}

int posix_write(int fd, char *ptr, int len)
{
    if (ptr)
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
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    FileDescriptor *pFd = pSubsystem->getFileDescriptor(fd);
    if (!pFd)
    {
        // Error - no such file descriptor.
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }

    // Copy to kernel.
    uint64_t nWritten = 0;
    if (ptr && len)
    {
        char *kernelBuf = new char[len];
        memcpy(reinterpret_cast<void*>(kernelBuf), reinterpret_cast<void*>(ptr), len);
        nWritten = pFd->file->write(pFd->offset, len, reinterpret_cast<uintptr_t>(kernelBuf));
        delete [] kernelBuf;
        pFd->offset += nWritten;
    }

    return static_cast<int>(nWritten);
}

off_t posix_lseek(int file, off_t ptr, int dir)
{
    F_NOTICE("lseek(" << file << ", " << ptr << ", " << dir << ")");

    // Lookup this process.
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    FileDescriptor *pFd = pSubsystem->getFileDescriptor(file);
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
    if (!f)
    {
        SYSCALL_ERROR(DoesNotExist);
        return -1;
    }

    if (!f->isSymlink())
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    if (buf == 0)
        return -1;

    HugeStaticString str;
    HugeStaticString tmp;
    str.clear();
    tmp.clear();
    NOTICE("Before followlink");
    return Symlink::fromFile(f)->followLink(buf, bufsize);
}

int posix_unlink(char *name)
{
    F_NOTICE("unlink(" << name << ")");

    if (VFS::instance().remove(String(name), GET_CWD()))
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
    while (src->isSymlink())
        src = Symlink::fromFile(src)->followLink();

    if (dest)
    {
        // traverse symlink
        while (dest->isSymlink())
            dest = Symlink::fromFile(dest)->followLink();

        if (dest->isDirectory() && !src->isDirectory())
        {
            SYSCALL_ERROR(FileExists);
            return -1;
        }
        else if (!dest->isDirectory() && src->isDirectory())
        {
            SYSCALL_ERROR(NotADirectory);
            return -1;
        }
    }
    else
    {
        VFS::instance().createFile(String(dst), 0777, GET_CWD());
        dest = VFS::instance().find(String(dst), GET_CWD());
        if (!dest)
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

    if (buf == 0)
        return 0;

    HugeStaticString str;
    HugeStaticString tmp;
    str.clear();
    tmp.clear();

    File* curr = GET_CWD();
    if (curr->getParent() != 0)
        str = curr->getName();

    File* f = curr;
    while ((f = f->getParent()))
    {
        // This feels a bit weird considering the while loop's subject...
        if (f->getParent())
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
    if (name[0] == 0)
    {
        SYSCALL_ERROR(DoesNotExist);
        return -1;
    }

    if(!st)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    File* file = 0;
    if (!strcmp(name, "/dev/null"))
        file = NullFs::instance().getFile();
    else
        file = VFS::instance().find(String(name), GET_CWD());
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
    if (ConsoleManager::instance().isConsole(file) || !strcmp(name, "/dev/null"))
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
    st->st_blksize = 1;
    st->st_blocks = (st->st_size / st->st_blksize) + ((st->st_size % st->st_blksize) ? 1 : 0);

    return 0;
}

int posix_fstat(int fd, struct stat *st)
{
    F_NOTICE("fstat(" << Dec << fd << Hex << ")");
    if(!st)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    // Lookup this process.
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    FileDescriptor *pFd = pSubsystem->getFileDescriptor(fd);
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
    st->st_blksize = 1;
    st->st_blocks = (st->st_size / st->st_blksize) + ((st->st_size % st->st_blksize) ? 1 : 0);

    return 0;
}

int posix_lstat(char *name, struct stat *st)
{
    F_NOTICE("lstat(" << name << ")");
    if(!st)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

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
    st->st_blksize = 1;
    st->st_blocks = (st->st_size / st->st_blksize) + ((st->st_size % st->st_blksize) ? 1 : 0);

    return 0;
}

int posix_opendir(const char *dir, dirent *ent)
{
    if(!dir)
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    F_NOTICE("opendir(" << dir << ")");

    // Lookup this process.
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    size_t fd = pSubsystem->getFd();

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
    if (file)
        ent->d_ino = file->getInode();
    else
    {
        // fail! no children!
        delete f;
        return -1;
    }

    pSubsystem->addFileDescriptor(fd, f);

    return static_cast<int>(fd);
}

int posix_readdir(int fd, dirent *ent)
{
    // F_NOTICE("readdir(" << fd << ")");

    if (fd == -1)
        return -1;

    // Lookup this process.
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    FileDescriptor *pFd = pSubsystem->getFileDescriptor(fd);
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
    if (fd == -1)
        return;

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return;
    }
    FileDescriptor *f = pSubsystem->getFileDescriptor(fd);
    f->offset = 0;
    posix_readdir(fd, ent);
}

int posix_closedir(int fd)
{
    if (fd == -1)
        return -1;

    /// \todo Race here - fix.
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    pSubsystem->freeFd(fd);

    return 0;
}

int posix_ioctl(int fd, int command, void *buf)
{
    F_NOTICE("ioctl(" << Dec << fd << ", " << Hex << command << ", " << reinterpret_cast<uintptr_t>(buf) << ")");

    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    FileDescriptor *f = pSubsystem->getFileDescriptor(fd);
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
            if (buf)
            {
                int a = *reinterpret_cast<int *>(buf);
                if (a)
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
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    FileDescriptor *f = pSubsystem->getFileDescriptor(fd);
    if (!f)
    {
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }

    size_t newFd = pSubsystem->getFd();

    // Copy the descriptor
    FileDescriptor* f2 = new FileDescriptor(*f);
    pSubsystem->addFileDescriptor(newFd, f2);

    return static_cast<int>(newFd);
}

int posix_dup2(int fd1, int fd2)
{
    F_NOTICE("dup2(" << fd1 << ", " << fd2 << ")");

    if (fd2 < 0)
    {
        SYSCALL_ERROR(BadFileDescriptor);
        return -1; // EBADF
    }

    if (fd1 == fd2)
        return fd2;

    // grab the file descriptor pointer for the passed descriptor
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    FileDescriptor* f = pSubsystem->getFileDescriptor(fd1);
    if (!f)
    {
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }

    // Copy the descriptor.
    //
    // This will also increase the refcount *before* we close the original, else we
    // might accidentally trigger an EOF condition on a pipe! (if the write refcount
    // drops to zero)...
    FileDescriptor* f2 = new FileDescriptor(*f);
    pSubsystem->addFileDescriptor(fd2, f2);

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
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    FileDescriptor *pFd = pSubsystem->getFileDescriptor(fd);
    if (!pFd)
    {
        // Error - no such file descriptor.
        ERROR("isatty: no such file descriptor (" << Dec << fd << ")");
        return 0;
    }

    NOTICE("isatty(" << fd << ") -> " << ((ConsoleManager::instance().isConsole(pFd->file)) ? 1 : 0));
    return (ConsoleManager::instance().isConsole(pFd->file)) ? 1 : 0;
}

/** SelectRun: performs a "select" call over a set of Files with a timeout. */
class SelectRun
{
    public:

        /// Information structure to make the Files list more useful
        struct FileInfo
        {
            File *pFile;
            bool bCheckWrite;
        };

        /// Default Constructor
        SelectRun(uint32_t timeout = 15) : m_Files(), m_Timeout(timeout)
        {};

        /// Destructor
        virtual ~SelectRun()
        {
            deleteAll();
        };

        /// Performs the actual run
        int doRun()
        {
            // Check each File in the list
            /// \todo This should keep looping until the timeout expires, this is currently just looping forever
            /// \todo These should each run concurrently, not one after the other...
            int numReady = 0;
            // while(numReady == 0)
            // {
                for(List<FileInfo*>::Iterator it = m_Files.begin(); it != m_Files.end(); it++)
                {
                    FileInfo *p = (*it);
                    if(p)
                        numReady += p->pFile->select(p->bCheckWrite, m_Timeout);
                }
            // }

            // Reset so we're ready for another round if needed
            deleteAll();
            return numReady;
        }

        /// Adds a file to the internal record
        void addFile(FileInfo *p)
        {
            m_Files.pushBack(p);
        }

        /// Sets the timeout
        void setTimeout(uint32_t t)
        {
            m_Timeout = t;
        }

    private:

        /// Deletes all internal FileInfo structures
        void deleteAll()
        {
            for(List<FileInfo*>::Iterator it = m_Files.begin(); it != m_Files.end(); it++)
            {
                FileInfo *p = (*it);
                if(p)
                    delete p;
            }
            m_Files.clear();
        }

        /// Internal record of Files that we are checking
        List<FileInfo*> m_Files;

        /// Timeout - is a soft deadline
        uint32_t m_Timeout;
};

/** poll: determine if a set of file descriptors are writable/readable.
 *
 *  Permits any number of descriptors, unlike select().
 *  \todo Timeout
 */
int posix_poll(struct pollfd* fds, unsigned int nfds, int timeout)
{
    F_NOTICE("poll(" << Dec << nfds << ", " << timeout << Hex << ")");

    // Grab the subsystem for this process
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Number of descriptors ready
    int numReady = 0;

    // Build the list
    for(unsigned int i = 0; i < nfds; i++)
    {
        struct pollfd *me = &fds[i];
        me->revents = 0;

        if(me->fd < 0)
            continue;

        FileDescriptor *pFd = pSubsystem->getFileDescriptor(me->fd);
        if(!pFd)
        {
            me->revents |= POLLNVAL;
            numReady++;
        }
        else
        {
            int n = 0;
            if(me->events & POLLIN)
            {
                if(pFd->file->select(false, 0))
                {
                    me->revents |= POLLIN;
                    n++;
                }
            }

            if(me->events & POLLOUT)
            {
                if(pFd->file->select(true, 0))
                {
                    me->revents |= POLLOUT;
                    n++;
                }
            }

            if(n)
            {
                me->revents |= POLLHUP | POLLERR | POLLNVAL;
                numReady++;
            }
        }
    }

    // Return the number ready to go
    return numReady;
}


/** select: determine if a set of file descriptors is readable, writable, or has an error condition.
 *
 *  Each descriptor should be checked asynchronously, in order to quickly handle large numbers of
 *  descriptors, and also to ensure that if the timeout condition expires the select() call returns.
 *
 *  The File::select function is used to direct the ugly details to the individual File. This allows
 *  things such as sockets to still use their very net-specific code without polluting select().
 */
int posix_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, timeval *timeout)
{
    F_NOTICE("select(" << Dec << nfds << Hex << ")");

    // Grab the subsystem for this process
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // This is the worker for this run
    SelectRun *run = new SelectRun;

    // Get the timeout
    uint32_t timeoutSeconds = 0;
    if (timeout)
        timeoutSeconds = timeout->tv_sec + (timeout->tv_usec / 1000);
    run->setTimeout(timeoutSeconds);

    // Setup the SelectRun object with all of the files
    for (int i = 0; i < nfds; i++)
    {
        // valid fd?
        FileDescriptor *pFd = 0;
        if ((readfds && FD_ISSET(i, readfds)) || (writefds && FD_ISSET(i, writefds)))
        {
            pFd = pSubsystem->getFileDescriptor(i);
            if (!pFd)
            {
                // Error - no such file descriptor.
                ERROR("select: no such file descriptor (" << Dec << i << ")");
                return -1;
            }
        }

        if (readfds)
        {
            if (FD_ISSET(i, readfds))
            {
                SelectRun::FileInfo *f = new SelectRun::FileInfo;
                f->pFile = pFd->file;
                f->bCheckWrite = false;

                run->addFile(f);
            }
        }
        if (writefds)
        {
            if (FD_ISSET(i, writefds))
            {
                SelectRun::FileInfo *f = new SelectRun::FileInfo;
                f->pFile = pFd->file;
                f->bCheckWrite = true;

                run->addFile(f);
            }
        }
    }

    // No descriptors but a timeout instead? Sleep.
    if(!nfds && timeoutSeconds)
    {
        Semaphore sem(0);
        sem.acquire(1, timeoutSeconds);
        return 0;
    }

    // Otherwise do the run
    int numReady = run->doRun();
    delete run;
    return numReady;
}

int posix_fcntl(int fd, int cmd, int num, int* args)
{
    if (num)
        F_NOTICE("fcntl(" << fd << ", " << cmd << ", " << num << ", " << args[0] << ")");
    else
        F_NOTICE("fcntl(" << fd << ", " << cmd << ")");

    // grab the file descriptor pointer for the passed descriptor
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    FileDescriptor* f = pSubsystem->getFileDescriptor(fd);
    if(!f)
    {
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }

    switch (cmd)
    {
        case F_DUPFD:

            if (num)
            {
                // if there is an argument and it's valid, map fd to the passed descriptor
                if (args[0] >= 0)
                {
                    size_t fd2 = static_cast<size_t>(args[0]);

                    // Copy the descriptor (addFileDescriptor automatically frees the old one, if needed)
                    FileDescriptor* f2 = new FileDescriptor(*f);
                    pSubsystem->addFileDescriptor(fd2, f2);

                    return static_cast<int>(fd2);
                }
            }
            else
            {
                size_t fd2 = pSubsystem->getFd();

                // copy the descriptor
                FileDescriptor* f2 = new FileDescriptor(*f);
                pSubsystem->addFileDescriptor(fd2, f2);

                return static_cast<int>(fd2);
            }
            return 0;
            break;

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
        case F_GETLK: // Get record-locking information
        case F_SETLK: // Set or clear a record lock (without blocking
        case F_SETLKW: // Set or clear a record lock (with blocking)

            // Grab the lock information structure
            struct flock *lock = reinterpret_cast<struct flock*>(args[0]);
            if(!lock)
            {
                SYSCALL_ERROR(InvalidArgument);
                return -1;
            }

            // Lock the LockedFile map
            // LockGuard<Mutex> lockFileGuard(g_PosixLockedFileMutex);

            // Can only take exclusive locks...
            if(cmd == F_GETLK)
            {
                if(f->lockedFile)
                {
                    lock->l_type = F_WRLCK;
                    lock->l_whence = SEEK_SET;
                    lock->l_start = lock->l_len = 0;
                    lock->l_pid = f->lockedFile->getLocker();
                }
                else
                    lock->l_type = F_UNLCK;

                return 0;
            }

            // Trying to set an exclusive lock?
            if(lock->l_type == F_WRLCK)
            {
                // Already got a LockedFile instance?
                if(f->lockedFile)
                {
                    if(cmd == F_SETLK)
                    {
                        return f->lockedFile->lock(false) ? 0 : -1;
                    }
                    else
                    {
                        // Lock the file, blocking
                        f->lockedFile->lock(true);
                        return 0;
                    }
                }

                // Not already locked!
                LockedFile *lf = new LockedFile(f->file);
                if(!lf)
                {
                    SYSCALL_ERROR(OutOfMemory);
                    return -1;
                }

                // Insert
                g_PosixGlobalLockedFiles.insert(f->file->getFullPath(), lf);
                f->lockedFile = lf;

                // The file is now locked
                return 0;
            }

            // Trying to unlock?
            if(lock->l_type == F_UNLCK)
            {
                // No locked file? Fail
                if(!f->lockedFile)
                    return -1;

                // Only need to unlock the file - it'll be locked again when needed
                f->lockedFile->unlock();
                return 0;
            }

            // Success, none of the above, no reason to be unlockable
            return 0;
    }

    SYSCALL_ERROR(Unimplemented);
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

void *posix_mmap(void *p)
{
    F_NOTICE("mmap");
    if(!p)
        return 0;

    // Grab the parameter list
    _mmap_tmp *map_info = reinterpret_cast<_mmap_tmp*>(p);

    // Get real variables from the parameters
    void *addr = map_info->addr;
    size_t len = map_info->len;
    int prot = map_info->prot;
    int flags = map_info->flags;
    int fd = map_info->fildes;
    off_t off = map_info->off;

    F_NOTICE("addr=" << reinterpret_cast<uintptr_t>(addr) << ", len=" << len << ", prot=" << prot << ", flags=" << flags << ", fildes=" << fd << ", off=" << off << ".");

    /// \todo Check args...

    // Get the File object to map
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return 0;
    }

    FileDescriptor* f = pSubsystem->getFileDescriptor(fd);
    if(!f)
    {
        SYSCALL_ERROR(BadFileDescriptor);
        return 0;
    }
    File *fileToMap = f->file;

    // Grab the MemoryMappedFile
    uintptr_t address = reinterpret_cast<uintptr_t>(addr);
    MemoryMappedFile *pFile = MemoryMappedFileManager::instance().map(fileToMap, address);

    // Add the offset...
    address += off;
    void *finalAddress = reinterpret_cast<void*>(address);

    // Map it in
    pSubsystem->memoryMapFile(finalAddress, pFile);

    // Complete
    return reinterpret_cast<void*>(finalAddress);
}

int posix_munmap(void *addr, size_t len)
{
    F_NOTICE("munmap");

    // Grab the Process subsystem
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    // Unmap the file (and grab it)
    MemoryMappedFile *pFile = pSubsystem->unmapFile(addr);
    if(pFile)
        MemoryMappedFileManager::instance().unmap(pFile);

    return 0;
}

int posix_access(const char *name, int amode)
{
    F_NOTICE("access(" << (name ? name : "n/a") << ", " << Dec << amode << Hex << ")");
    if(!name)
    {
        SYSCALL_ERROR(DoesNotExist);
        return -1;
    }

    // Grab the file
    File *file = VFS::instance().find(String(name), GET_CWD());
    if (!file)
    {
        SYSCALL_ERROR(DoesNotExist);
        return -1;
    }

    /// \todo Proper permission checks. For now, the file exists, and you can do what you want with it.
    return 0;
}


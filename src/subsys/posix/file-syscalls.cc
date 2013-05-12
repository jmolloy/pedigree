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
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <process/Process.h>
#include <utilities/Tree.h>
#include <vfs/File.h>
#include <vfs/LockedFile.h>
#include <vfs/MemoryMappedFile.h>
#include <vfs/Symlink.h>
#include <vfs/Directory.h>
#include <vfs/VFS.h>
#include <console/Console.h>
#include <network-stack/NetManager.h>
#include <network-stack/Tcp.h>
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

inline File *traverseSymlink(File *file)
{
    if(!file)
    {
        SYSCALL_ERROR(DoesNotExist);
        return 0;
    }

    Tree<File*, File*> loopDetect;
    while(file->isSymlink())
    {
        file = Symlink::fromFile(file)->followLink();
        if(!file)
        {
            SYSCALL_ERROR(DoesNotExist);
            return 0;
        }

        if(loopDetect.lookup(file))
        {
            SYSCALL_ERROR(LoopExists);
            return 0;
        }
        else
            loopDetect.insert(file, file);
    }

    return file;
}

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

    F_NOTICE("open(" << name << ", " << ((mode & O_RDWR) ? "O_RDWR" : "") << ((mode & O_RDONLY) ? "O_RDONLY" : "") << ((mode & O_WRONLY) ? "O_WRONLY" : "") << ")");
    
    // One of these three must be specified
    /// \bug Breaks /dev/tty open in crt0
#if 0
    if(!(flags & (O_RDONLY | O_WRONLY | O_RDWR)))
    {
        F_NOTICE("One of O_RDONLY, O_WRONLY, or O_RDWR must be passed.");
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }
#endif

    // verify the filename - don't try to open a dud file
    if (name[0] == 0)
    {
        F_NOTICE("File does not exist (null path).");
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
                F_NOTICE("File does not exist (createFile failed)");
                SYSCALL_ERROR(DoesNotExist);
                pSubsystem->freeFd(fd);
                return -1;
            }

            file = VFS::instance().find(String(name), GET_CWD());
            if (!file)
            {
                F_NOTICE("File does not exist (O_CREAT failed)");
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
    else if(flags & O_CREAT)
    {
        if(flags & O_EXCL)
        {
            F_NOTICE("File exists, exclusive mode is on, and O_CREAT was set.");
            SYSCALL_ERROR(FileExists);
            return -1;
        }
    }

    if(!file)
    {
      F_NOTICE("File does not exist.");
      SYSCALL_ERROR(DoesNotExist);
      pSubsystem->freeFd(fd);
      return -1;
    }

    file = traverseSymlink(file);

    if(!file)
        return -1;

    if (file->isDirectory() && (flags & (O_WRONLY | O_RDWR)))
    {
        // Error - is directory.
        F_NOTICE("Is a directory, and O_WRONLY or O_RDWR was specified.");
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
      F_NOTICE("File does not exist.");
      SYSCALL_ERROR(DoesNotExist);
      pSubsystem->freeFd(fd);
      return -1;
    }

    F_NOTICE("    -> " << fd);

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
    
    if(pFd->file->isDirectory())
    {
        SYSCALL_ERROR(IsADirectory);
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

    F_NOTICE("    -> " << Dec << nRead << Hex);

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

int posix_symlink(char *target, char *link)
{
    F_NOTICE("symlink(" << target << ", " << link << ")");

    bool worked = VFS::instance().createSymlink(String(link), String(target), GET_CWD());
    if (worked)
        return 0;
    else
        ERROR("Symlink failed for `" << link << "' -> `" << target << "'");
    return -1;
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
    src = traverseSymlink(src);
    if(!src)
        return -1;

    if (dest)
    {
        // traverse symlink
        dest = traverseSymlink(dest);
        if(!dest)
            return -1;

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
    delete [] buf;

    return 0;
}

char* posix_getcwd(char* buf, size_t maxlen)
{
    F_NOTICE("getcwd(" << maxlen << ")");

    if (buf == 0)
        return 0;

    File* curr = GET_CWD();
    String str = curr->getFullPath();

    size_t maxLength = str.length();
    if(maxLength > maxlen)
        maxLength = maxlen;
    strncpy(buf, static_cast<const char*>(str), maxlen);

    return buf;
}

int posix_stat(const char *name, struct stat *st)
{
    F_NOTICE("stat(" << name << ")");

    // verify the filename - don't try to open a dud file (otherwise we'll open the cwd)
    if (name[0] == 0)
    {
        F_NOTICE("    -> Doesn't exist");
        SYSCALL_ERROR(DoesNotExist);
        return -1;
    }

    if(!st)
    {
        F_NOTICE("    -> Invalid argument");
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
        F_NOTICE("    -> Not found by VFS");
        SYSCALL_ERROR(DoesNotExist);
        return -1;
    }
    
    file = traverseSymlink(file);

    if(!file)
    {
        F_NOTICE("    -> Symlink traversal failed");
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

    F_NOTICE("    -> Success");
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
        ERROR("Error, no such FD!");
        // Error - no such file descriptor.
        return -1;
    }

    int mode = 0;
    if (ConsoleManager::instance().isConsole(pFd->file))
    {
        F_NOTICE("    -> S_IFCHR");
        mode = S_IFCHR;
    }
    else if (pFd->file->isDirectory())
    {
        F_NOTICE("    -> S_IFDIR");
        mode = S_IFDIR;
    }
    else
    {
        F_NOTICE("    -> S_IFREG");
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
    F_NOTICE("    -> " << mode);

    st->st_dev   = static_cast<short>(reinterpret_cast<uintptr_t>(pFd->file->getFilesystem()));
    F_NOTICE("    -> " << st->st_dev);
    st->st_ino   = static_cast<short>(pFd->file->getInode());
    F_NOTICE("    -> " << st->st_ino);
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

    F_NOTICE("Success");
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
    
    file = traverseSymlink(file);

    if(!file)
        return -1;

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
    {
        ent->d_ino = file->getInode();

        // Some applications consider a null inode to mean "bad file" which is
        // a horrible assumption for them to make. Because the presence of a file
        // is indicated by more effective means (ie, successful return from
        // readdir) this just appeases the applications which aren't portably
        // written.
        if(ent->d_ino == 0)
            ent->d_ino = 0x7fff; // Signed, don't want this to turn negative

        // Copy the filename across
        strncpy(ent->d_name, static_cast<const char*>(file->getName()), MAXNAMLEN);
    }
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
    F_NOTICE("readdir(" << fd << ")");

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
    if (!pFd || !pFd->file)
    {
        // Error - no such file descriptor.
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }

    if(!pFd->file->isDirectory())
    {
        SYSCALL_ERROR(NotADirectory);
        return -1;
    }
    File* file = Directory::fromFile(pFd->file)->getChild(pFd->offset);
    if (!file)
    {
        // Normal EOF condition.
        SYSCALL_ERROR(NoError);
        return -1;
    }

    ent->d_ino = static_cast<short>(file->getInode());
    String tmp = file->getName();
    strcpy(ent->d_name, static_cast<const char*>(tmp));
    ent->d_name[strlen(static_cast<const char*>(tmp))] = '\0';
    if(file->isSymlink())
        ent->d_type = DT_LNK;
    else
        ent->d_type = file->isDirectory() ? DT_DIR : DT_REG;
    pFd->offset ++;

    return 0;
}

void posix_rewinddir(int fd, dirent *ent)
{
    if (fd == -1)
        return;

    F_NOTICE("rewinddir(" << fd << ")");

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

    F_NOTICE("closedir(" << fd << ")");

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

        case TIOCFLUSH:
        {
            return console_flush(f->file, buf);
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
    if (dir && dir->isDirectory())
    {
        Processor::information().getCurrentThread()->getParent()->setCwd(dir);

        char tmp[1024];
        posix_getcwd(tmp, 1024);
        return 0;
    }
    else if(dir && !dir->isDirectory())
    {
        SYSCALL_ERROR(NotADirectory);
        return -1;
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

    // According to the spec, CLOEXEC is cleared on DUP.
    f2->fdflags &= ~FD_CLOEXEC;

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

int posix_fcntl(int fd, int cmd, int num, int* args)
{
    if (num)
        F_NOTICE("fcntl(" << fd << ", " << cmd << ", " << num << ", " << args[0] << ")");
    /// \note Added braces. Compiler warned about ambiguity if F_NOTICE isn't enabled. It seems to be able
    ///       to figure out what we meant to do, but making it explicit never hurt anyone.
    else
    {
        F_NOTICE("fcntl(" << fd << ", " << cmd << ")");
    }

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

                    // According to the spec, CLOEXEC is cleared on DUP.
                    f2->fdflags &= ~FD_CLOEXEC;

                    return static_cast<int>(fd2);
                }
            }
            else
            {
                size_t fd2 = pSubsystem->getFd();

                // copy the descriptor
                FileDescriptor* f2 = new FileDescriptor(*f);
                pSubsystem->addFileDescriptor(fd2, f2);

                // According to the spec, CLOEXEC is cleared on DUP.
                f2->fdflags &= ~FD_CLOEXEC;

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

    // Get the File object to map
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return MAP_FAILED;
    }

    // The return address
    void *finalAddress = 0;

    // Valid file passed?
    FileDescriptor* f = pSubsystem->getFileDescriptor(fd);
    if(flags & MAP_ANON)
    {
        // Anonymous mmap instead?
        if(flags & (MAP_ANON | MAP_SHARED))
        {
            // Allocation information
            VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
            uintptr_t mapAddress = reinterpret_cast<uintptr_t>(addr);
            size_t pageSz = PhysicalMemoryManager::getPageSize();
            size_t numPages = (len / pageSz) + (len % pageSz ? 1 : 0);

            // Verify the passed length
            if(!len || (mapAddress & (pageSz-1)))
            {
                SYSCALL_ERROR(InvalidArgument);
                return MAP_FAILED;
            }

            // Does the application want a fixed mapping?
            // A fixed mapping is a direct mapping into the address space by an
            // application.
            if(flags & MAP_FIXED)
            {
                // Verify the passed address
                if(!mapAddress)
                {
                    SYSCALL_ERROR(InvalidArgument);
                    return MAP_FAILED;
                }

                // Unmap existing allocations (before releasing the space to the process'
                // space allocator though).
                /// \todo Could this wreak havoc with CoW or shared memory (when we get it)?
                for (size_t i = 0; i < numPages; i++)
                {
                    void *unmapAddr = reinterpret_cast<void*>(mapAddress + (i * pageSz));
                    if(va.isMapped(unmapAddr))
                    {
                        // Unmap the virtual address
                        physical_uintptr_t phys = 0;
                        size_t flags = 0;
                        va.getMapping(unmapAddr, phys, flags);
                        va.unmap(reinterpret_cast<void*>(unmapAddr));

                        // Free the physical page
                        PhysicalMemoryManager::instance().freePage(phys);
                    }
                }

                // Now, allocate the memory
                if(!pProcess->getSpaceAllocator().allocateSpecific(mapAddress, numPages * pageSz))
                {
                    if(!(flags & MAP_USERSVD))
                    {
                        F_NOTICE("mmap: out of memory");
                        SYSCALL_ERROR(OutOfMemory);
                        return MAP_FAILED;
                    }
                }
            }
            else if(flags & (MAP_ANON | MAP_SHARED))
            {
                // Anonymous mapping - get an address from the space allocator
                /// \todo NEED AN ALIGNED ADDRESS. COME ON.
                if(!pProcess->getSpaceAllocator().allocate(numPages * pageSz, mapAddress))
                {
                    F_NOTICE("mmap: out of memory");
                    SYSCALL_ERROR(OutOfMemory);
                    return MAP_FAILED;
                }

                if (mapAddress & (pageSz-1))
                {
                    mapAddress = (mapAddress + pageSz) & ~(pageSz - 1);
                }
            }
            else
            {
                // Flags not supported
                F_NOTICE("mmap: flags not supported");
                SYSCALL_ERROR(NotSupported);
                return MAP_FAILED;
            }

            // Got an address and a length, map it in now
            for(size_t i = 0; i < numPages; i++)
            {
                physical_uintptr_t  phys = PhysicalMemoryManager::instance().allocatePage();
                uintptr_t           virt = mapAddress + (i * pageSz);
                if(!va.isMapped(reinterpret_cast<void*>(virt)))
                {
                    if(!va.map(phys, reinterpret_cast<void*>(virt), VirtualAddressSpace::Write))
                    {
                        /// \todo Need to unmap and free all the mappings so far, then return to the space allocator.
                        F_NOTICE("mmap: out of memory");
                        SYSCALL_ERROR(OutOfMemory);
                        return MAP_FAILED;
                    }
                }
                else
                {
                    // Page is already mapped, that shouldn't be possible.
                    FATAL("mmap: address " << virt << " is already mapped into the address space");
                }
            }

            // And finally, we're ready to go.
            finalAddress = reinterpret_cast<void*>(mapAddress);

            // Add to the tracker
            /// \todo This is not really ideal - rather than storing a MMFile
            ///       we should store an object of some description. Then it'll
            ///       be something like pSubsystem->addMemoryMap()
            MemoryMappedFile *pFile = new MemoryMappedFile(len);
            pSubsystem->memoryMapFile(finalAddress, pFile);

            // Clear the allocated region if needed
            if(flags & MAP_ANON)
            {
                memset(finalAddress, 0, numPages * PhysicalMemoryManager::getPageSize());
            }
        }
        else
        {
            // No valid file given, return error
            F_NOTICE("mmap: invalid file descriptor");
            SYSCALL_ERROR(BadFileDescriptor);
            return MAP_FAILED;
        }
    }
    else
    {
        // Grab the file to map in
        File *fileToMap = f->file;
        
        F_NOTICE("mmap: file name is " << fileToMap->getFullPath());

        // Grab the MemoryMappedFile for it. This will automagically handle
        // MAP_FIXED mappings too
        /// \todo There *should* be proper flag checks here!
        uintptr_t address = reinterpret_cast<uintptr_t>(addr);
        bool bShared = (flags & MAP_SHARED);
        MemoryMappedFile *pFile = MemoryMappedFileManager::instance().map(fileToMap, address, len, off, bShared);

        // Add the offset...
        // address += off;
        finalAddress = reinterpret_cast<void*>(address);

        // Another memory mapped file to keep track of
        pSubsystem->memoryMapFile(finalAddress, pFile);
    }

    // Complete
    return finalAddress;
}

int posix_msync(void *p, size_t len, int flags) {
    F_NOTICE("msync");

    // Hacky, more or less just for the dynamic linker to figure out if a page is already mapped.
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    if(!va.isMapped(p)) {
        SYSCALL_ERROR(OutOfMemory);
        return -1;
    }

    return 0;
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

    // Lookup the address
    MemoryMappedFile *pFile = pSubsystem->unmapFile(addr);

    // If it's a valid file, unmap it and we're done
    if(pFile)
        MemoryMappedFileManager::instance().unmap(pFile);

    // Otherwise, free the space manually
    else
    {
        // Anonymous mmap, manually free the space
        size_t pageSz = PhysicalMemoryManager::getPageSize();
        size_t numPages = (len / pageSz) + (len % pageSz ? 1 : 0);

        uintptr_t address = reinterpret_cast<uintptr_t>(addr);

        // Unmap!
        VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
        for (size_t i = 0; i < numPages; i++)
        {
            void *unmapAddr = reinterpret_cast<void*>(address + (i * pageSz));
            if(va.isMapped(unmapAddr))
            {
                // Unmap the virtual address
                physical_uintptr_t phys = 0;
                size_t flags = 0;
                va.getMapping(unmapAddr, phys, flags);
                va.unmap(reinterpret_cast<void*>(unmapAddr));

                // Free the physical page
                PhysicalMemoryManager::instance().freePage(phys);
            }
        }

        // Free from the space allocator as well
        pProcess->getSpaceAllocator().free(address, len);
    }

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

int posix_ftruncate(int a, off_t b)
{
	F_NOTICE("ftruncate(" << a << ", " << b << ")");

    // Grab the File pointer for this file
    Process *pProcess = Processor::information().getCurrentThread()->getParent();
    PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
    if (!pSubsystem)
    {
        ERROR("No subsystem for this process!");
        return -1;
    }

    FileDescriptor *pFd = pSubsystem->getFileDescriptor(a);
    if (!pFd)
    {
        // Error - no such file descriptor.
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }
    File *pFile = pFd->file;

    // If we are to simply truncate, do so
    if(b == 0)
    {
        pFile->truncate();
        return 0;
    }
    else if(static_cast<size_t>(b) == pFile->getSize())
        return 0;
    // If we need to reduce the file size, do so
    else if(static_cast<size_t>(b) < pFile->getSize())
    {
        pFile->setSize(b);
        return 0;
    }
    // Otherwise, extend the file
    else
    {
        size_t currSize = pFile->getSize();
        size_t numExtraBytes = b - currSize;
        NOTICE("Extending by " << numExtraBytes << " bytes");
        uint8_t *nullBuffer = new uint8_t[numExtraBytes];
        NOTICE("Got the buffer");
        memset(nullBuffer, 0, numExtraBytes);
        NOTICE("Zeroed the buffer");
        pFile->write(currSize, numExtraBytes, reinterpret_cast<uintptr_t>(nullBuffer));
        NOTICE("Deleting the buffer");
        delete [] nullBuffer;
        NOTICE("Complete");
        return 0;
    }

    // Can't get here
	return -1;
}

int pedigree_get_mount(char* mount_buf, char* info_buf, size_t n)
{
    NOTICE("pedigree_get_mount(" << Dec << n << Hex << ")");
    
    typedef List<String*> StringList;
    typedef Tree<Filesystem *, List<String*>* > VFSMountTree;
    VFSMountTree &mounts = VFS::instance().getMounts();
    
    size_t i = 0;
    for(VFSMountTree::Iterator it = mounts.begin();
        it != mounts.end();
        it++)
    {
        Filesystem *pFs = it.key();
        StringList *pList = it.value();
        Disk *pDisk = pFs->getDisk();
        
        for(StringList::Iterator it2 = pList->begin();
            it2 != pList->end();
            it2++, i++)
        {
            String mount = **it2;
            
            if(i == n)
            {
                String info, s;
                if(pDisk)
                {
                    pDisk->getName(s);
                    pDisk->getParent()->getName(info);
                    info += " // ";
                    info += s;
                }
                else
                    info = "no disk";
                
                strcpy(mount_buf, static_cast<const char *>(mount));
                strcpy(info_buf, static_cast<const char *>(info));
                
                return 0;
            }
        }
    }
    
    return -1;
}

int posix_chmod(const char *path, mode_t mode)
{
    F_NOTICE("chmod(" << String(path) << ", " << Oct << mode << Hex << ")");
    
    /// \todo EACCESS, EPERM
    
    if((mode == static_cast<mode_t>(-1)) || (mode > 0777))
    {
        SYSCALL_ERROR(InvalidArgument);
        return -1;
    }

    File* file = VFS::instance().find(String(path), GET_CWD());
    if (!file)
    {
        SYSCALL_ERROR(DoesNotExist);
        return -1;
    }
    
    // Read-only filesystem?
    if(file->getFilesystem()->isReadOnly())
    {
        SYSCALL_ERROR(ReadOnlyFilesystem);
        return -1;
    }

    // Symlink traversal
    file = traverseSymlink(file);
    if(!file)
        return -1;
    
    /// \todo Might want to change permissions on open file descriptors?
    uint32_t permissions = 0;
    if (mode & S_IRUSR) permissions |= FILE_UR;
    if (mode & S_IWUSR) permissions |= FILE_UW;
    if (mode & S_IXUSR) permissions |= FILE_UX;
    if (mode & S_IRGRP) permissions |= FILE_GR;
    if (mode & S_IWGRP) permissions |= FILE_GW;
    if (mode & S_IXGRP) permissions |= FILE_GX;
    if (mode & S_IROTH) permissions |= FILE_OR;
    if (mode & S_IWOTH) permissions |= FILE_OW;
    if (mode & S_IXOTH) permissions |= FILE_OX;
    file->setPermissions(permissions);
    
    return 0;
}

int posix_chown(const char *path, uid_t owner, gid_t group)
{
    F_NOTICE("chown(" << String(path) << ", " << owner << ", " << group << ")");
    
    /// \todo EACCESS, EPERM
    
    // Is there any need to change?
    if((owner == group) && (owner == static_cast<uid_t>(-1)))
        return 0;

    File* file = VFS::instance().find(String(path), GET_CWD());
    if (!file)
    {
        SYSCALL_ERROR(DoesNotExist);
        return -1;
    }
    
    // Read-only filesystem?
    if(file->getFilesystem()->isReadOnly())
    {
        SYSCALL_ERROR(ReadOnlyFilesystem);
        return -1;
    }

    // Symlink traversal
    file = traverseSymlink(file);
    if(!file)
        return -1;
    
    // Set the UID and GID
    if(owner != static_cast<uid_t>(-1))
        file->setUid(owner);
    if(group != static_cast<gid_t>(-1))
        file->setGid(group);
    
    return 0;
}

int posix_fchmod(int fd, mode_t mode)
{
    F_NOTICE("fchmod(" << fd << ", " << Oct << mode << Hex << ")");
    
    /// \todo EACCESS, EPERM
    
    if((mode == static_cast<mode_t>(-1)) || (mode > 0777))
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
        SYSCALL_ERROR(BadFileDescriptor);
        return -1;
    }
    
    File *file = pFd->file;
    
    // Read-only filesystem?
    if(file->getFilesystem()->isReadOnly())
    {
        SYSCALL_ERROR(ReadOnlyFilesystem);
        return -1;
    }
    
    /// \todo Might want to change permissions on open file descriptors?
    uint32_t permissions = 0;
    if (mode & S_IRUSR) permissions |= FILE_UR;
    if (mode & S_IWUSR) permissions |= FILE_UW;
    if (mode & S_IXUSR) permissions |= FILE_UX;
    if (mode & S_IRGRP) permissions |= FILE_GR;
    if (mode & S_IWGRP) permissions |= FILE_GW;
    if (mode & S_IXGRP) permissions |= FILE_GX;
    if (mode & S_IROTH) permissions |= FILE_OR;
    if (mode & S_IWOTH) permissions |= FILE_OW;
    if (mode & S_IXOTH) permissions |= FILE_OX;
    file->setPermissions(permissions);
    
    return 0;
}

int posix_fchown(int fd, uid_t owner, gid_t group)
{
    F_NOTICE("fchown(" << fd << ", " << owner << ", " << group << ")");
    
    /// \todo EACCESS, EPERM
    
    // Is there any need to change?
    if((owner == group) && (owner == static_cast<uid_t>(-1)))
        return 0;
    
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
    
    File *file = pFd->file;
    
    // Read-only filesystem?
    if(file->getFilesystem()->isReadOnly())
    {
        SYSCALL_ERROR(ReadOnlyFilesystem);
        return -1;
    }
    
    // Set the UID and GID
    if(owner != static_cast<uid_t>(-1))
        file->setUid(owner);
    if(group != static_cast<gid_t>(-1))
        file->setGid(group);
    
    return 0;
}

int posix_fchdir(int fd)
{
    F_NOTICE("fchdir(" << fd << ")");
    
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
    
    File *file = pFd->file;
    if(!file->isDirectory())
    {
        SYSCALL_ERROR(NotADirectory);
        return -1;
    }
    
    Processor::information().getCurrentThread()->getParent()->setCwd(file);
    return 0;
}

int statvfs_doer(Filesystem *pFs, struct statvfs *buf)
{
    if(!pFs)
    {
        SYSCALL_ERROR(DoesNotExist);
        return -1;
    }
    
    /// \todo Get all this data from the Filesystem object
    buf->f_bsize = 4096;
    buf->f_frsize = 512;
    buf->f_blocks = static_cast<fsblkcnt_t>(-1);
    buf->f_bfree = static_cast<fsblkcnt_t>(-1);
    buf->f_bavail = static_cast<fsblkcnt_t>(-1);
    buf->f_files = 0;
    buf->f_ffree = static_cast<fsfilcnt_t>(-1);
    buf->f_favail = static_cast<fsfilcnt_t>(-1);
    buf->f_fsid = 0;
    buf->f_flag = (pFs->isReadOnly() ? ST_RDONLY : 0) | ST_NOSUID; // No suid in pedigree yet.
    buf->f_namemax = VFS_MNAMELEN;
    
    // FS type
    strcpy(buf->f_fstypename, "ext2");
    
    // "From" point
    /// \todo Disk device hash + path (on raw filesystem maybe?)
    strcpy(buf->f_mntfromname, "from");
    
    // "To" point
    /// \todo What to put here?
    strcpy(buf->f_mntfromname, "to");
    
    return 0;
}

int posix_fstatvfs(int fd, struct statvfs *buf)
{
    F_NOTICE("fstatvfs(" << fd << ")");
    
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
    
    File *file = pFd->file;

    return statvfs_doer(file->getFilesystem(), buf);
}

int posix_statvfs(const char *path, struct statvfs *buf)
{
    F_NOTICE("statvfs(" << path << ")");

    File* file = VFS::instance().find(String(path), GET_CWD());
    if (!file)
    {
        SYSCALL_ERROR(DoesNotExist);
        return -1;
    }

    // Symlink traversal
    file = traverseSymlink(file);
    if(!file)
        return -1;
    
    return statvfs_doer(file->getFilesystem(), buf);
}


/*
 * Copyright (c) 2013 Matthew Iselin
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

#ifndef _UNIX_FILESYSTEM_H
#define _UNIX_FILESYSTEM_H

#include <vfs/Filesystem.h>
#include <vfs/File.h>
#include <vfs/Directory.h>

#include <utilities/RingBuffer.h>

#define MAX_UNIX_DGRAM_BACKLOG   65536

/**
 * UnixFilesystem: UNIX sockets.
 *
 * This filesystem is mounted with the "unix" 'volume' label, and provides
 * the filesystem abstraction for UNIX sockets (at least, non-anonymous ones).
 */
class UnixFilesystem : public Filesystem
{
    public:
        UnixFilesystem();
        virtual ~UnixFilesystem();

        virtual bool initialise(Disk *pDisk)
        {
            return false;
        }

        virtual File *getRoot()
        {
            return m_pRoot;
        }

        virtual String getVolumeLabel()
        {
            return String("unix");
        }

        virtual uint64_t read(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true)
        {
            return pFile->read(location, size, buffer, bCanBlock);
        }
        virtual uint64_t write(File *pFile, uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true)
        {
            return pFile->write(location, size, buffer, bCanBlock);
        }

        virtual void truncate(File *pFile)
        {
        }

        virtual void fileAttributeChanged(File *pFile)
        {
        }

        virtual void cacheDirectoryContents(File *pFile)
        {
            if(pFile->isDirectory())
            {
                Directory *pDir = Directory::fromFile(pFile);
                pDir->cacheDirectoryContents();
            }
        }

        virtual void extend(File *pFile, size_t size)
        {
        }

    protected:
        virtual bool createFile(File *parent, String filename, uint32_t mask);
        virtual bool createDirectory(File *parent, String filename);
        virtual bool createSymlink(File *parent, String filename, String value)
        {
            return false;
        }
        virtual bool remove(File *parent, File *file);

    private:
        File *m_pRoot;
};

/**
 * A UNIX socket.
 */
class UnixSocket : public File
{
    public:
        UnixSocket(String name, Filesystem *pFs, File *pParent);
        virtual ~UnixSocket();

        virtual uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);
        virtual uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock = true);

        virtual int select(bool bWriting = false, int timeout = 0);

    private:
        struct buf {
            char *pBuffer;
            uint64_t len;
            char *remotePath; // Path of the socket that dumped data here, if any.
        };

        /// \todo stream sockets
        RingBuffer<struct buf *> m_RingBuffer;
};

/**
 * Basic Directory subclass for UNIX socket support.
 */
class UnixDirectory : public Directory
{
    public:
        UnixDirectory(String name, Filesystem *pFs, File *pParent);
        virtual ~UnixDirectory();

        bool addEntry(String filename, File *pFile);
        bool removeEntry(File *pFile);

        virtual void cacheDirectoryContents();

    private:
        Mutex m_Lock;
};

#endif


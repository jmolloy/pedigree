/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#include "Ext2Directory.h"
#include "Ext2Filesystem.h"
#include <syscallError.h>
#include "Ext2File.h"
#include "Ext2Symlink.h"

Ext2Directory::Ext2Directory(String name, uintptr_t inode_num, Inode *inode,
                             Ext2Filesystem *pFs, File *pParent) :
    Directory(name, LITTLE_TO_HOST32(inode->i_atime),
              LITTLE_TO_HOST32(inode->i_mtime),
              LITTLE_TO_HOST32(inode->i_ctime),
              inode_num,
              static_cast<Filesystem*>(pFs),
              LITTLE_TO_HOST32(inode->i_size), /// \todo Deal with >4GB files here.
              pParent),
    Ext2Node(inode_num, inode, pFs)
{
    uint32_t mode = LITTLE_TO_HOST32(inode->i_mode);
    uint32_t permissions = 0;
    if (mode & EXT2_S_IRUSR) permissions |= FILE_UR;
    if (mode & EXT2_S_IWUSR) permissions |= FILE_UW;
    if (mode & EXT2_S_IXUSR) permissions |= FILE_UX;
    if (mode & EXT2_S_IRGRP) permissions |= FILE_GR;
    if (mode & EXT2_S_IWGRP) permissions |= FILE_GW;
    if (mode & EXT2_S_IXGRP) permissions |= FILE_GX;
    if (mode & EXT2_S_IROTH) permissions |= FILE_OR;
    if (mode & EXT2_S_IWOTH) permissions |= FILE_OW;
    if (mode & EXT2_S_IXOTH) permissions |= FILE_OX;

    setPermissions(permissions);
    setUid(LITTLE_TO_HOST16(inode->i_uid));
    setGid(LITTLE_TO_HOST16(inode->i_gid));
}

Ext2Directory::~Ext2Directory()
{
}

bool Ext2Directory::addEntry(String filename, File *pFile, size_t type)
{
    // Calculate the size of our Dir* entry.
    size_t length = 4 + /* 32-bit inode number */
        2 + /* 16-bit record length */
        1 + /* 8-bit name length */
        1 + /* 8-bit file type */
        filename.length(); /* Don't leave space for NULL-terminator, not needed. */

    // Load in and scan the directory, looking for a space large enough to fit our directory descriptor.
    uint8_t *pBuffer = new uint8_t[m_pExt2Fs->m_BlockSize];

    bool bFound = false;

    uint32_t i;
    Dir *pDir;
    for (i = 0; i < m_nBlocks; i++)
    {
        ensureBlockLoaded(i);
        uintptr_t buffer = m_pExt2Fs->readBlock(m_pBlocks[i]);
        pDir = reinterpret_cast<Dir*>(buffer);
        while (reinterpret_cast<uintptr_t>(pDir) < buffer+m_pExt2Fs->m_BlockSize)
        {
            // What's the minimum length of this directory entry?
            size_t thisReclen = 4 + 2 + 1 + 1 + pDir->d_namelen;

            // Valid directory entry?
            if (pDir->d_inode > 0)
            {
                // Is there enough space to add this dirent?
                /// \todo Ensure 4-byte alignment.
                if (pDir->d_reclen - thisReclen >= length)
                {
                    bFound = true;
                    // Save the current reclen.
                    uint16_t oldReclen = pDir->d_reclen;
                    // Adjust the current record's reclen field to the minimum.
                    pDir->d_reclen = thisReclen;
                    // Move to the new directory entry location.
                    pDir = reinterpret_cast<Dir*> (reinterpret_cast<uintptr_t>(pDir)+thisReclen);
                    // set the new record length.
                    pDir->d_reclen = oldReclen-thisReclen;
                    break;
                }
            }
            else if (pDir->d_reclen == 0)
            {
                // No more entries to follow.
                break;
            }
            else if (pDir->d_reclen - thisReclen >= length)
            {
                // We can use this unused entry - we fit into it.
                // The record length does not need to be adjusted.
                bFound = true;
                break;
            }

            // Next.
            pDir = reinterpret_cast<Dir*> (reinterpret_cast<uintptr_t>(pDir)+pDir->d_reclen);
        }
        if (bFound) break;
    }

    if (!bFound)
    {
        // Need to make a new block.
        uint32_t block = m_pExt2Fs->findFreeBlock(getInodeNumber());
        if (block == 0)
        {
            // We had a problem.
            SYSCALL_ERROR(NoSpaceLeftOnDevice);
            return false;
        }
        if (!addBlock(block)) return false;
        i = m_nBlocks-1;

        m_Size = m_nBlocks * m_pExt2Fs->m_BlockSize;
        fileAttributeChanged();

        /// \todo Previous directory entry might need its reclen updated to
        ///       point to this new entry (as directory entries cannot cross
        ///       block boundaries).

        memset(pBuffer, 0, m_pExt2Fs->m_BlockSize);
        pDir = reinterpret_cast<Dir*> (pBuffer);
        pDir->d_reclen = m_pExt2Fs->m_BlockSize;
    }

    // Set the directory contents.
    pDir->d_inode = HOST_TO_LITTLE32(pFile->getInode());

    switch (type)
    {
        case EXT2_S_IFREG:
            pDir->d_file_type = EXT2_FILE;
            break;
        case EXT2_S_IFDIR:
            pDir->d_file_type = EXT2_DIRECTORY;
            break;
        case EXT2_S_IFLNK:
            pDir->d_file_type = EXT2_SYMLINK;
            break;
        default:
            ERROR("Unrecognised filetype.");
    }

    pDir->d_namelen = filename.length();
    memcpy(pDir->d_name, filename, filename.length());

    // We're all good - add the directory to our cache.
    m_Cache.insert(filename, pFile);

    m_Size = m_nSize;

    return true;
}

bool Ext2Directory::removeEntry(const String &filename, Ext2Node *pFile)
{
    // Find this file in the directory.
    size_t fileInode = pFile->getInodeNumber();

    NOTICE("ext2: remove " << filename);
    NOTICE("inode: " << pFile->getInodeNumber());

    bool bFound = false;

    uint32_t i;
    Dir *pDir;
    for (i = 0; i < m_nBlocks; i++)
    {
        ensureBlockLoaded(i);
        uintptr_t buffer = m_pExt2Fs->readBlock(m_pBlocks[i]);
        pDir = reinterpret_cast<Dir*>(buffer);
        while (reinterpret_cast<uintptr_t>(pDir) < buffer + m_pExt2Fs->m_BlockSize)
        {
            NOTICE("inode: " << LITTLE_TO_HOST32(pDir->d_inode) << " vs " << fileInode);
            if (LITTLE_TO_HOST32(pDir->d_inode) == fileInode)
            {
                NOTICE("namelen: " << pDir->d_namelen << " vs " << filename.length());
                if (pDir->d_namelen == filename.length())
                {
                    if (!strncmp(pDir->d_name, static_cast<const char *>(filename), pDir->d_namelen))
                    {
                        // Wipe out the directory entry.
                        uint16_t old_reclen = pDir->d_reclen;
                        memset(pDir, 0, sizeof(Dir));
                        pDir->d_reclen = old_reclen;

                        /// \todo Okay, this is not quite enough. The previous
                        ///       entry needs to be updated to skip past this
                        ///       now-empty entry. If this was the first entry,
                        ///       a blank record must be created to point to
                        ///       either the next entry or the end of the block.

                        bFound = true;
                        break;
                    }
                }
            }
            else if (!pDir->d_reclen)
            {
                // No more entries.
                NOTICE("ext2: end of directory block");
                break;
            }
            pDir = reinterpret_cast<Dir*> (reinterpret_cast<uintptr_t>(pDir) + LITTLE_TO_HOST16(pDir->d_reclen));
        }

        if (bFound) break;
    }

    m_Size = m_nSize;

    if (bFound)
    {
        return true;
    }
    else
    {
        SYSCALL_ERROR(DoesNotExist);
        return false;
    }
}

void Ext2Directory::cacheDirectoryContents()
{
    uint32_t i;
    Dir *pDir;
    for (i = 0; i < m_nBlocks; i++)
    {
        ensureBlockLoaded(i);
        uintptr_t buffer = m_pExt2Fs->readBlock(m_pBlocks[i]);
        pDir = reinterpret_cast<Dir*>(buffer);

        while (reinterpret_cast<uintptr_t>(pDir) < buffer+m_pExt2Fs->m_BlockSize)
        {
            Dir *pNextDir = reinterpret_cast<Dir*> (reinterpret_cast<uintptr_t>(pDir) + LITTLE_TO_HOST16(pDir->d_reclen));

            if (pDir->d_inode == 0)
            {
                if (pDir == pNextDir)
                {
                    // No further iteration possible (null entry).
                    break;
                }

                // Oops, not a valid entry (possibly deleted file). Skip.
                pDir = pNextDir;
                continue;

            }

            size_t namelen = pDir->d_namelen + 1;

            // Can we get the file type from the directory entry?
            size_t fileType = EXT2_UNKNOWN;
            if (m_pExt2Fs->checkRequiredFeature(2))
            {
                // Directory entry holds file type.
                fileType = pDir->d_file_type;
            }
            else
            {
                // Inode holds file type.
                Inode *inode = m_pExt2Fs->getInode(pDir->d_inode);
                size_t inode_ftype = inode->i_mode & 0xF000;
                switch (inode_ftype)
                {
                    case EXT2_S_IFLNK:
                        fileType = EXT2_SYMLINK;
                        break;
                    case EXT2_S_IFREG:
                        fileType = EXT2_FILE;
                        break;
                    case EXT2_S_IFDIR:
                        fileType = EXT2_DIRECTORY;
                        break;
                    default:
                        ERROR("EXT2: Inode has unsupported file type: " << inode_ftype << ".");
                        break;
                }

                // In this case, the file type entry is the top 8 bits of the
                // filename length.
                namelen |= pDir->d_file_type << 8;
            }

            // Grab filename from the entry.
            char *filename = new char[namelen];
            memcpy(filename, pDir->d_name, namelen - 1);
            filename[namelen - 1] = '\0';
            String sFilename(filename);
            delete [] filename;

            NOTICE("file " << sFilename << " has inode " << LITTLE_TO_HOST32(pDir->d_inode));

            uint32_t inode = LITTLE_TO_HOST32(pDir->d_inode);

            File *pFile = 0;
            switch (fileType)
            {
                case EXT2_FILE:
                    {
                    pFile = new Ext2File(sFilename, inode, m_pExt2Fs->getInode(inode), m_pExt2Fs, this);
                    Ext2File *pE = reinterpret_cast<Ext2File *>(pFile);
                    NOTICE("file " << sFilename << " has inodes " << inode << ", " << pE->getInodeNumber() << ", " << pFile->getInode());
                    }
                    break;
                case EXT2_DIRECTORY:
                    pFile = new Ext2Directory(sFilename, inode, m_pExt2Fs->getInode(inode), m_pExt2Fs, this);
                    break;
                case EXT2_SYMLINK:
                    pFile = new Ext2Symlink(sFilename, inode, m_pExt2Fs->getInode(inode), m_pExt2Fs, this);
                    break;
                default:
                    ERROR("EXT2: Unrecognised file type for '" << sFilename << "': " << pDir->d_file_type);
            }

            // Add to cache.
            m_Cache.insert(sFilename, pFile);

            // Next.
            pDir = pNextDir;
        }
    }

    m_bCachePopulated = true;
}

void Ext2Directory::fileAttributeChanged()
{
    static_cast<Ext2Node*>(this)->fileAttributeChanged(m_Size, m_AccessedTime, m_ModifiedTime, m_CreationTime);
}

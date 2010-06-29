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

#include "Ext2Directory.h"
#include "Ext2File.h"
#include "Ext2Filesystem.h"
#include "Ext2Symlink.h"
#include <Log.h>
#include <Module.h>
#include <machine/Machine.h>
#include <machine/Timer.h>
#include <process/Process.h>
#include <processor/Processor.h>
#include <syscallError.h>
#include <users/UserManager.h>
#include <utilities/List.h>
#include <utilities/StaticString.h>
#include <utilities/assert.h>
#include <utilities/utility.h>
#include <vfs/VFS.h>

// The sparse block page. This is zeroed and made read-only. A handler is set and if
// written, it traps.
/// \todo Set CR0.WP bit else this will never happen.
/// \todo Work out what to do when it traps.
uint8_t g_pSparseBlock[4096] ALIGN(4096) __attribute__((__section__(".bss")));


Ext2Filesystem::Ext2Filesystem() :
    m_pSuperblock(0), m_pGroupDescriptors(), m_BlockSize(0), m_InodeSize(0),
    m_nGroupDescriptors(0), m_WriteLock(false), m_pRoot(0)
{
}

Ext2Filesystem::~Ext2Filesystem()
{
    if(m_pRoot)
        delete m_pRoot;
}

bool Ext2Filesystem::initialise(Disk *pDisk)
{
    m_pDisk = pDisk;

    // Attempt to read the superblock.
    uintptr_t block = m_pDisk->read(1024ULL);
    m_pSuperblock = reinterpret_cast<Superblock*>(block);

    // Read correctly?
    if (LITTLE_TO_HOST16(m_pSuperblock->s_magic) != 0xEF53)
    {
        String devName;
        pDisk->getName(devName);
        ERROR("Ext2: Superblock not found on device " << devName);
        return false;
    }

    // Calculate the block size.
    m_BlockSize = 1024 << LITTLE_TO_HOST32(m_pSuperblock->s_log_block_size);

    // More than 4096 bytes per block and we're a little screwed atm.
    assert(m_BlockSize <= 4096);

    // Calculate the inode size.
    /// \todo Check for EXT2_DYNAMIC_REV - only applicable for dynamic revision.
    m_InodeSize = LITTLE_TO_HOST16(m_pSuperblock->s_inode_size);

    // Where is the group descriptor table?
    uint32_t gdBlock = LITTLE_TO_HOST32(m_pSuperblock->s_first_data_block)+1;

    // How many group descriptors do we have?
    m_nGroupDescriptors = LITTLE_TO_HOST32(m_pSuperblock->s_inodes_count) / LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group);

    // Add an entry to the group descriptor tree for each GD.
    m_pGroupDescriptors = new GroupDesc*[m_nGroupDescriptors];
    for (size_t i = 0; i < m_nGroupDescriptors; i++)
    {
        uintptr_t idx = (i * sizeof(GroupDesc)) / m_BlockSize;
        uintptr_t off = (i * sizeof(GroupDesc)) % m_BlockSize;

        uintptr_t block = readBlock(gdBlock+idx);
        m_pGroupDescriptors[i] = reinterpret_cast<GroupDesc*>(block+off);
    }

    // Create our bitmap arrays and tables.
    m_pInodeTables  = new Vector<size_t>[m_nGroupDescriptors];
    m_pInodeBitmaps = new Vector<size_t>[m_nGroupDescriptors];
    m_pBlockBitmaps = new Vector<size_t>[m_nGroupDescriptors];

    /// \todo Set g_pSparseBlock as read-only.

    return true;
}

Filesystem *Ext2Filesystem::probe(Disk *pDisk)
{
    Ext2Filesystem *pFs = new Ext2Filesystem();
    if (!pFs->initialise(pDisk))
    {
        /// \todo Why's this bad boy here?
        // delete pFs;
        return 0;
    }
    else
        return pFs;
}

File* Ext2Filesystem::getRoot()
{
    if (!m_pRoot)
    {
        Inode *inode = getInode(EXT2_ROOT_INO);
        m_pRoot = new Ext2Directory(String(""), EXT2_ROOT_INO, inode, this, 0);
    }
    return m_pRoot;
}

String Ext2Filesystem::getVolumeLabel()
{
    if (m_pSuperblock->s_volume_name[0] == '\0')
    {
        NormalStaticString str;
        str += "no-volume-label@";
        str.append(reinterpret_cast<uintptr_t>(this), 16);
        return String(static_cast<const char*>(str));
    }
    char buffer[17];
    strncpy(buffer, m_pSuperblock->s_volume_name, 16);
    buffer[16] = '\0';
    return String(buffer);
}

bool Ext2Filesystem::createNode(File* parent, String filename, uint32_t mask, String value, size_t type)
{
    // Quick sanity check;
    if (!parent->isDirectory())
    {
        SYSCALL_ERROR(NotADirectory);
        return false;
    }

    // The filename cannot be the special entries "." or "..".
    if (filename.length() == 0 || !strcmp(filename, ".") || !strcmp(filename, ".."))
    {
        SYSCALL_ERROR(InvalidArgument);
        return false;
    }

    // Find a free inode.
    uint32_t inode_num = findFreeInode();
    if (inode_num == 0)
    {
        SYSCALL_ERROR(NoSpaceLeftOnDevice);
        return false;
    }

    Timer *pTimer = Machine::instance().getTimer();

    // Populate the inode.
    /// \todo Endianness!
    Inode *newInode = getInode(inode_num);
    memset(reinterpret_cast<uint8_t*>(newInode), 0, sizeof(Inode));
    newInode->i_mode = mask | type;
    newInode->i_uid = Processor::information().getCurrentThread()->getParent()->getUser()->getId();
    newInode->i_atime = newInode->i_ctime = newInode->i_mtime = pTimer->getUnixTimestamp();
    newInode->i_gid = Processor::information().getCurrentThread()->getParent()->getGroup()->getId();
    newInode->i_links_count = 1;

    // If we have a value to store, and it's small enough, use the block indices.
    if (value.length() && value.length() < 4*15)
    {
        memcpy(reinterpret_cast<void*>(newInode->i_block), value, value.length());
        newInode->i_size = value.length();
    }
    // Else case comes later, after pFile is created.

    Ext2Directory *pE2Parent = reinterpret_cast<Ext2Directory*>(parent);

    // Create the new File object.
    File *pFile = 0;
    switch (type)
    {
        case EXT2_S_IFREG:
            pFile = new Ext2File(filename, inode_num, newInode, this, parent);
            break;
        case EXT2_S_IFDIR:
        {
            Ext2Directory *pE2Dir = new Ext2Directory(filename, inode_num, newInode, this, parent);
            pFile = pE2Dir;
            pE2Dir->addEntry(String(".."), pE2Parent, EXT2_S_IFDIR);
            pE2Dir->addEntry(String("."), pFile, EXT2_S_IFDIR);
            break;
        }
        case EXT2_S_IFLNK:
            pFile = new Ext2Symlink(filename, inode_num, newInode, this, parent);
            break;
        default:
            FATAL("EXT2: Unrecognised file type: " << Hex << type);
            break;
    }

    // Else case from earlier.
    if (value.length() && value.length() >= 4*15)
    {
        const char *pStr = value;
        pFile->write(0ULL, value.length(), reinterpret_cast<uintptr_t>(pStr));
    }

    // Add to the parent directory.
    if (!pE2Parent->addEntry(filename, pFile, type))
    {
        ERROR("EXT2: Internal error adding directory entry.");
        SYSCALL_ERROR(IoError);
        return false;
    }

    // Edit the atime and mtime of the parent directory.
    parent->setAccessedTime(pTimer->getUnixTimestamp());
    parent->setModifiedTime(pTimer->getUnixTimestamp());

    return true;
}

bool Ext2Filesystem::createFile(File *parent, String filename, uint32_t mask)
{
    return createNode(parent, filename, mask, String(""), EXT2_S_IFREG);
}

bool Ext2Filesystem::createDirectory(File* parent, String filename)
{
    if (!createNode(parent, filename, 0, String(""), EXT2_S_IFDIR))
        return false;
    return true;
}

bool Ext2Filesystem::createSymlink(File* parent, String filename, String value)
{
    return createNode(parent, filename, 0, value, EXT2_S_IFLNK);
}

bool Ext2Filesystem::remove(File* parent, File* file)
{
    return true;
}

uintptr_t Ext2Filesystem::readBlock(uint32_t block)
{
    if (block == 0)
        return reinterpret_cast<uintptr_t>(g_pSparseBlock);

    return m_pDisk->read(static_cast<uint64_t>(m_BlockSize)*static_cast<uint64_t>(block));
}

uint32_t Ext2Filesystem::findFreeBlock(uint32_t inode)
{
    inode--; // Inode zero is undefined, so it's not used.

    uint32_t group = inode / LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group);
    group = 0;

    for (; group < m_nGroupDescriptors; group++)
    {
        ///\todo Check group descriptor hint.
        ensureFreeBlockBitmapLoaded(group);

        Vector<size_t> &list = m_pBlockBitmaps[group];
        for (size_t i = 0;
             i < LITTLE_TO_HOST32(m_pSuperblock->s_blocks_per_group)/8;
             i += sizeof(uint32_t))
        {
            // Calculate block index.
            size_t idx = i / (m_BlockSize*8);
            size_t off = i % (m_BlockSize*8);

            // Grab the specific block.
            /// \todo Endianness - to ensure correct operation, must ptr be
            /// little endian?
            uintptr_t block = list[idx];
            uint32_t *ptr = reinterpret_cast<uint32_t*> (block+off);
            uint32_t tmp = *ptr;

            if (tmp == ~0U)
                continue;

            for (size_t j = 0; j < 32; j++,tmp >>= 1)
            {
                if ( (tmp&1) == 0 )
                {
                    // Unused, we can use this block!
                    // Set it as used.
                    *ptr = *ptr | (1<<j);
                    ///\todo Update the group descriptor's free_blocks_count.
                    return (group * LITTLE_TO_HOST32(m_pSuperblock->s_blocks_per_group)) +
                        i*8 + j;
                }
            }

            // Shouldn't get here - if there were no available blocks here it should
            // have hit the "continue" above!
            assert(false);
        }
    }

    return 0;
}

uint32_t Ext2Filesystem::findFreeInode()
{
    for (uint32_t group = 0; group < m_nGroupDescriptors; group++)
    {
        ///\todo Check group descriptor hint.
        ensureFreeInodeBitmapLoaded(group);

        Vector<size_t> &list = m_pInodeBitmaps[group];
        for (size_t i = 0;
             i < LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group)/8;
             i += sizeof(uint32_t))
        {
            // Calculate block index.
            size_t idx = i / (m_BlockSize*8);
            size_t off = i % (m_BlockSize*8);

            // Grab the specific block.
            /// \todo Endianness - to ensure correct operation, must ptr be
            /// little endian?
            uintptr_t block = list[idx];
            uint32_t *ptr = reinterpret_cast<uint32_t*> (block+off);
            uint32_t tmp = *ptr;

            if (tmp == ~0U)
                continue;

            for (size_t j = 0; j < 32; j++,tmp >>= 1)
            {
                if ( (tmp&1) == 0 )
                {
                    // Unused, we can use this block!
                    // Set it as used.
                    *ptr = *ptr | (1<<j);
                    ///\todo Update the group descriptor's free_inodes_count.
                    return (group * LITTLE_TO_HOST32(m_pSuperblock->s_blocks_per_group)) +
                        i*8 + j;
                }
            }

            // Shouldn't get here - if there were no available blocks here it should
            // have hit the "continue" above!
            assert(false);
        }
    }

    return 0;
}

void Ext2Filesystem::releaseBlock(uint32_t block)
{
}

Inode *Ext2Filesystem::getInode(uint32_t inode)
{
    inode--; // Inode zero is undefined, so it's not used.

    uint32_t group = inode / LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group);
    uint32_t index = inode % LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group);

    ensureInodeTableLoaded(group);
    Vector<size_t> &list = m_pInodeTables[group];

    size_t blockNum = (index*sizeof(Inode)) / m_BlockSize;
    size_t blockOff = (index*sizeof(Inode)) % m_BlockSize;

    uintptr_t block = list[blockNum];

    return reinterpret_cast<Inode*> (block+blockOff);
}

void Ext2Filesystem::ensureFreeBlockBitmapLoaded(size_t group)
{
    assert(group < m_nGroupDescriptors);
    Vector<size_t> &list = m_pBlockBitmaps[group];

    if (list.size() > 0)
        // Descriptors already loaded.
        return;

    size_t nBlocks = LITTLE_TO_HOST32(m_pSuperblock->s_blocks_per_group) / (m_BlockSize*8);
    if (LITTLE_TO_HOST32(m_pSuperblock->s_blocks_per_group) % (m_BlockSize*8))
        nBlocks ++;

    for (size_t i = 0; i < nBlocks; i++)
    {
        list.pushBack(
            readBlock(LITTLE_TO_HOST32(m_pGroupDescriptors[group]->bg_block_bitmap)+i));
    }
}

void Ext2Filesystem::ensureFreeInodeBitmapLoaded(size_t group)
{
    assert(group < m_nGroupDescriptors);
    Vector<size_t> &list = m_pInodeBitmaps[group];

    if (list.size() > 0)
        // Descriptors already loaded.
        return;

    size_t nBlocks = LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group) / (m_BlockSize*8);
    if (LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group) % (m_BlockSize*8))
        nBlocks ++;

    for (size_t i = 0; i < nBlocks; i++)
    {
        list.pushBack(
            readBlock(LITTLE_TO_HOST32(m_pGroupDescriptors[group]->bg_inode_bitmap)+i));
    }
}

void Ext2Filesystem::ensureInodeTableLoaded(size_t group)
{
    assert(group < m_nGroupDescriptors);
    Vector<size_t> &list = m_pInodeTables[group];

    if (list.size() > 0)
        // Descriptors already loaded.
        return;

    size_t nBlocks = (LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group)*sizeof(Inode)) / m_BlockSize;
    if ((LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group)*sizeof(Inode)) / m_BlockSize)
        nBlocks ++;

    for (size_t i = 0; i < nBlocks; i++)
    {
        list.pushBack(
            readBlock(LITTLE_TO_HOST32(m_pGroupDescriptors[group]->bg_inode_table)+i));
    }
}

static void initExt2()
{
    VFS::instance().addProbeCallback(&Ext2Filesystem::probe);
}

static void destroyExt2()
{
}

MODULE_INFO("ext2", &initExt2, &destroyExt2, "vfs");

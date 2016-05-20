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
#include "Ext2File.h"
#include "Ext2Filesystem.h"
#include "Ext2Symlink.h"
#include <Log.h>
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

#ifndef EXT2_STANDALONE
#include <Module.h>
#endif

// The sparse block page. This is zeroed and made read-only. A handler is set and if
// written, it traps.
/// \todo Set CR0.WP bit else this will never happen.
/// \todo Work out what to do when it traps.
static uint8_t g_pSparseBlock[4096] ALIGN(4096) SECTION(".bss");

#ifdef EXT2_STANDALONE
extern uint32_t getUnixTimestamp();
#else
static uint32_t getUnixTimestamp()
{
    Timer *pTimer = Machine::instance().getTimer();
    return pTimer->getUnixTimestamp();
}
#endif


Ext2Filesystem::Ext2Filesystem() :
    m_pSuperblock(0), m_pGroupDescriptors(0), m_pInodeTables(0),
    m_pInodeBitmaps(0), m_pBlockBitmaps(0), m_BlockSize(0), m_InodeSize(0),
    m_nGroupDescriptors(0),
#ifdef THREADS
    m_WriteLock(false),
#endif
    m_pRoot(0)
{
}

Ext2Filesystem::~Ext2Filesystem()
{
    delete [] m_pBlockBitmaps;
    delete [] m_pInodeBitmaps;
    delete [] m_pInodeTables;
    delete [] m_pGroupDescriptors;
    delete m_pRoot;
}

bool Ext2Filesystem::initialise(Disk *pDisk)
{
    String devName;
    m_pDisk = pDisk;
    pDisk->getName(devName);

    // Attempt to read the superblock.
    // We need to pin the block, as we'll hold onto it.
    uintptr_t block = m_pDisk->read(1024ULL);
    if (!block || block == ~static_cast<uintptr_t>(0U))
    {
        return false;
    }
    m_pDisk->pin(1024ULL);
    m_pSuperblock = reinterpret_cast<Superblock*>(block);

    // Read correctly?
    if (LITTLE_TO_HOST16(m_pSuperblock->s_magic) != 0xEF53)
    {
        ERROR("Ext2: Superblock not found on device " << devName);
        m_pDisk->unpin(1024ULL);
        return false;
    }

    // Clean?
    if (LITTLE_TO_HOST16(m_pSuperblock->s_state) != EXT2_STATE_CLEAN)
    {
        WARNING("Ext2: filesystem on device " << devName << " is not clean.");
    }

    // Compressed filesystem?
    if (checkRequiredFeature(1))
    {
        WARNING("Ext2: filesystem on device " << devName << " requires compression, some files may fail to read.");

        // Compression type.
        uint32_t algo_bitmap = LITTLE_TO_HOST32(m_pSuperblock->s_algo_bitmap);
        switch(algo_bitmap)
        {
            case EXT2_LZV1_ALG:
                NOTICE("Ext2: filesystem on device '" << devName << "' uses compression algorithm LZV1.");
                break;
            case EXT2_LZRW3A_ALG:
                NOTICE("Ext2: filesystem on device '" << devName << "' uses compression algorithm LZRW3A.");
                break;
            case EXT2_GZIP_ALG:
                NOTICE("Ext2: filesystem on device '" << devName << "' uses compression algorithm gzip.");
                break;
            case EXT2_BZIP2_ALG:
                NOTICE("Ext2: filesystem on device '" << devName << "' uses compression algorithm bzip2.");
                break;
            case EXT2_LZO_ALG:
                NOTICE("Ext2: filesystem on device '" << devName << "' uses compression algorithm LZO.");
                break;
            default:
                ERROR("Ext2: unknown compression algorithm " << algo_bitmap << " on device '" << devName << "' -- cannot mount!");
                return false;
        }
    }

    /// \todo Check for journal required features.
    /// \todo Check all read-only features.

    // If we can, check extended superblock fields.
    if (LITTLE_TO_HOST32(m_pSuperblock->s_rev_level) >= 1)
    {
        // Non-standard inode sizes are permitted, handle that.
        m_InodeSize = LITTLE_TO_HOST16(m_pSuperblock->s_inode_size);
    }
    else
    {
        m_InodeSize = sizeof(Inode);
    }

    // Calculate the block size.
    m_BlockSize = 1024 << LITTLE_TO_HOST32(m_pSuperblock->s_log_block_size);

    // More than 4096 bytes per block and we're a little screwed atm.
    assert(m_BlockSize <= 4096);

    // Where is the group descriptor table?
    uint32_t gdBlock = LITTLE_TO_HOST32(m_pSuperblock->s_first_data_block) + 1;

    // How many group descriptors do we have? Round up the result.
    uint32_t inodeCount = LITTLE_TO_HOST32(m_pSuperblock->s_inodes_count);
    uint32_t inodesPerGroup = LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group);
    m_nGroupDescriptors = (inodeCount / inodesPerGroup) + (inodeCount % inodesPerGroup);

    // Add an entry to the group descriptor tree for each GD.
    m_pGroupDescriptors = new GroupDesc*[m_nGroupDescriptors];
    for (size_t i = 0; i < m_nGroupDescriptors; i++)
    {
        uintptr_t idx = (i * sizeof(GroupDesc)) / m_BlockSize;
        uintptr_t off = (i * sizeof(GroupDesc)) % m_BlockSize;

        uintptr_t groupBlock = readBlock(gdBlock+idx);
        m_pGroupDescriptors[i] = reinterpret_cast<GroupDesc*>(groupBlock+off);
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
        // No ext2 filesystem found - don't leak the filesystem object.
        delete pFs;
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
    bool hasVolumeLabel = LITTLE_TO_HOST32(m_pSuperblock->s_rev_level) >= 1;
    if ((!hasVolumeLabel) || (m_pSuperblock->s_volume_name[0] == '\0'))
    {
        NormalStaticString str;
        str += "no-volume-label@";
        str.append(reinterpret_cast<uintptr_t>(this), 16);
        return String(static_cast<const char*>(str));
    }
    else
    {
        char buffer[17];
        StringCopyN(buffer, m_pSuperblock->s_volume_name, 16);
        buffer[16] = '\0';
        return String(buffer);
    }
}

bool Ext2Filesystem::createNode(File* parent, String filename, uint32_t mask, String value, size_t type, uint32_t inodeOverride)
{
    NOTICE("CREATE: " << filename);

    // Quick sanity check;
    if (!parent->isDirectory())
    {
        SYSCALL_ERROR(NotADirectory);
        return false;
    }

    // The filename cannot be the special entries "." or "..".
    if (filename.length() == 0 || !StringCompare(filename, ".") || !StringCompare(filename, ".."))
    {
        SYSCALL_ERROR(InvalidArgument);
        return false;
    }

    // Find a free inode.
    uint32_t inode_num = inodeOverride;
    if (!inode_num)
    {
        inode_num = findFreeInode();
        if (inode_num == 0)
        {
            SYSCALL_ERROR(NoSpaceLeftOnDevice);
            return false;
        }
    }

#ifdef EXT2_STANDALONE
    size_t uid = 0;
    size_t gid = 0;
#else
    size_t uid = Processor::information().getCurrentThread()->getParent()->getUser()->getId();
    size_t gid = Processor::information().getCurrentThread()->getParent()->getGroup()->getId();
#endif

    uint32_t timestamp = getUnixTimestamp();

    // Populate the inode.
    /// \todo Endianness!
    Inode *newInode = getInode(inode_num);
    if (!inodeOverride)
    {
        ByteSet(reinterpret_cast<uint8_t*>(newInode), 0, m_InodeSize);
        newInode->i_mode = HOST_TO_LITTLE16(mask | type);
        newInode->i_uid = HOST_TO_LITTLE16(uid);
        newInode->i_atime = newInode->i_ctime = newInode->i_mtime = HOST_TO_LITTLE32(timestamp);
        newInode->i_gid = HOST_TO_LITTLE16(gid);
    }

    // If we have a value to store, and it's small enough, use the block indices.
    if (value.length() && value.length() < 4*15)
    {
        MemoryCopy(reinterpret_cast<void*>(newInode->i_block), value, value.length());
        newInode->i_size = HOST_TO_LITTLE32(value.length());
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

            // If we already have an inode, assume we already have dot/dotdot
            // entries and so don't need to make them.
            if (!inodeOverride)
            {
                Inode *parentInode = getInode(pE2Parent->getInodeNumber());

                // Create dot and dotdot entries.
                Ext2Directory *pDot = new Ext2Directory(String("."), inode_num, newInode, this, pE2Dir);
                Ext2Directory *pDotDot = new Ext2Directory(String(".."), pE2Parent->getInodeNumber(), parentInode, this, pE2Dir);

                // Add created dot/dotdot entries to the new directory.
                pE2Dir->addEntry(String("."), pDot, EXT2_S_IFDIR);
                pE2Dir->addEntry(String(".."), pDotDot, EXT2_S_IFDIR);
            }
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
    parent->setAccessedTime(timestamp);
    parent->setModifiedTime(timestamp);

    // Write updated inodes.
    writeInode(inode_num);
    writeInode(pE2Parent->getInodeNumber());

    // Update directory count in the group descriptor.
    if (type == EXT2_S_IFDIR)
    {
        uint32_t group = (inode_num - 1) / LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group);
        GroupDesc *pDesc = m_pGroupDescriptors[group];

        pDesc->bg_used_dirs_count++;

        // Update group descriptor on disk.
        /// \todo save group descriptor block number elsewhere
        uint32_t gdBlock = LITTLE_TO_HOST32(m_pSuperblock->s_first_data_block) + 1;
        uint32_t groupBlock = (group * sizeof(GroupDesc)) / m_BlockSize;
        writeBlock(gdBlock + groupBlock);
    }

    return true;
}

bool Ext2Filesystem::createFile(File *parent, String filename, uint32_t mask)
{
    return createNode(parent, filename, mask, String(""), EXT2_S_IFREG);
}

bool Ext2Filesystem::createDirectory(File* parent, String filename)
{
    // rwxr-xr-x - sane initial permissions for directory.
    /// \todo Hard-coding permissions here sucks.
    if (!createNode(parent, filename, 0755, String(""), EXT2_S_IFDIR))
        return false;
    return true;
}

bool Ext2Filesystem::createSymlink(File* parent, String filename, String value)
{
    return createNode(parent, filename, 0777, value, EXT2_S_IFLNK);
}

bool Ext2Filesystem::createLink(File* parent, String filename, File *target)
{
    Ext2Directory *pE2Parent = reinterpret_cast<Ext2Directory*>(parent);

    Ext2Node *pNode = 0;
    if (target->isDirectory())
    {
        Ext2Directory *pDirectory = static_cast<Ext2Directory *>(target);
        pNode = pDirectory;
    }
    else if (target->isSymlink())
    {
        Ext2Symlink *pSymlink = static_cast<Ext2Symlink *>(target);
        pNode = pSymlink;
    }
    else
    {
        Ext2File *pFile = static_cast<Ext2File *>(target);
        pNode = pFile;
    }

    if (!pNode)
    {
        return false;
    }

    // Extract permissions and entry type.
    Inode *inode = pNode->getInode();
    uint32_t mask = LITTLE_TO_HOST16(inode->i_mode) & 0x0FFF;
    size_t type = LITTLE_TO_HOST16(inode->i_mode) & 0xF000;

    return createNode(parent, filename, mask, String(""), type, pNode->getInodeNumber());
}

bool Ext2Filesystem::remove(File* parent, File* file)
{
    // Quick sanity check.
    if (!parent->isDirectory())
    {
        SYSCALL_ERROR(IoError);
        return false;
    }

    Ext2Node *pNode = 0;
    String filename;
    if (file->isDirectory())
    {
        Ext2Directory *pDirectory = static_cast<Ext2Directory *>(file);
        pNode = pDirectory;
        filename = pDirectory->getName();
    }
    else if (file->isSymlink())
    {
        Ext2Symlink *pSymlink = static_cast<Ext2Symlink *>(file);
        pNode = pSymlink;
        filename = pSymlink->getName();
    }
    else
    {
        Ext2File *pFile = static_cast<Ext2File *>(file);
        pNode = pFile;
        filename = pFile->getName();
    }

    NOTICE("REMOVE: " << filename);

    Ext2Directory *pE2Parent = reinterpret_cast<Ext2Directory*>(parent);
    bool result = pE2Parent->removeEntry(filename, pNode);

    // Update the group descriptor directory count to reflect the deletion.
    if (result && file->isDirectory() && (filename != "." && filename != ".."))
    {
        uint32_t inode_num = pNode->getInodeNumber();

        uint32_t group = (inode_num - 1) / LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group);
        GroupDesc *pDesc = m_pGroupDescriptors[group];

        pDesc->bg_used_dirs_count--;

        // Update group descriptor on disk.
        /// \todo save group descriptor block number elsewhere
        uint32_t gdBlock = LITTLE_TO_HOST32(m_pSuperblock->s_first_data_block) + 1;
        uint32_t groupBlock = (group * sizeof(GroupDesc)) / m_BlockSize;
        writeBlock(gdBlock + groupBlock);
    }

    return result;
}

uintptr_t Ext2Filesystem::readBlock(uint32_t block)
{
    if (block == 0)
        return reinterpret_cast<uintptr_t>(g_pSparseBlock);

    return m_pDisk->read(static_cast<uint64_t>(m_BlockSize)*static_cast<uint64_t>(block));
}

void Ext2Filesystem::writeBlock(uint32_t block)
{
    if (block == 0)
        return;

    m_pDisk->write(static_cast<uint64_t>(m_BlockSize) * static_cast<uint64_t>(block));
}

void Ext2Filesystem::pinBlock(uint64_t location)
{
    m_pDisk->pin(static_cast<uint64_t>(m_BlockSize) * location);
}

void Ext2Filesystem::unpinBlock(uint64_t location)
{
    m_pDisk->unpin(static_cast<uint64_t>(m_BlockSize) * location);
}

void Ext2Filesystem::sync(size_t offset, bool async)
{
    if (async)
        m_pDisk->write(static_cast<uint64_t>(m_BlockSize) * offset);
    else
        m_pDisk->flush(static_cast<uint64_t>(m_BlockSize) * offset);
}

uint32_t Ext2Filesystem::findFreeBlock(uint32_t inode)
{
    inode--; // Inode zero is undefined, so it's not used.

    uint32_t group = inode / LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group);

    for (; group < m_nGroupDescriptors; group++)
    {
        // Any free blocks here?
        GroupDesc *pDesc = m_pGroupDescriptors[group];
        if (!pDesc->bg_free_blocks_count)
        {
            // No blocks free in this group.
            continue;
        }

        ensureFreeBlockBitmapLoaded(group);

        // 8 blocks per byte - i == bitmap offset in bytes.
        Vector<size_t> &list = m_pBlockBitmaps[group];
        for (size_t i = 0;
             i < LITTLE_TO_HOST32(m_pSuperblock->s_blocks_per_group) / 8;
             i += sizeof(uint32_t))
        {
            // Calculate block index into the bitmap.
            size_t idx = i / (m_BlockSize * 8);
            size_t off = i % (m_BlockSize * 8);

            // Grab the specific block for the bitmap.
            /// \todo Endianness - to ensure correct operation, must ptr be
            /// little endian?
            uintptr_t block = list[idx];
            uint32_t *ptr = reinterpret_cast<uint32_t*> (block + off);
            uint32_t tmp = *ptr;

            // Bitmap full of bits? Skip it.
            if (tmp == ~0U)
                continue;

            // Check each bit in this field.
            for (size_t j = 0; j < 32; j++, tmp >>= 1)
            {
                // Free?
                if ((tmp & 1) == 0)
                {
                    // This block is free! Mark used.
                    *ptr |= (1 << j);
                    pDesc->bg_free_blocks_count--;

                    // Update superblock.
                    m_pSuperblock->s_free_blocks_count--;
                    m_pDisk->write(1024ULL);

                    // Update bitmap on disk.
                    uint32_t desc_block = LITTLE_TO_HOST32(m_pGroupDescriptors[group]->bg_block_bitmap) + idx;
                    writeBlock(desc_block);

                    // Update group descriptor on disk.
                    /// \todo save group descriptor block number elsewhere
                    uint32_t gdBlock = LITTLE_TO_HOST32(m_pSuperblock->s_first_data_block) + 1;
                    uint32_t groupBlock = (group * sizeof(GroupDesc)) / m_BlockSize;
                    writeBlock(gdBlock + groupBlock);

                    // First block of this group...
                    uint32_t result = group * LITTLE_TO_HOST32(m_pSuperblock->s_blocks_per_group);
                    // Add the data block offset for this filesystem.
                    result += LITTLE_TO_HOST32(m_pSuperblock->s_first_data_block);
                    // Blocks skipped so far (i == offset in bytes)...
                    result += i * 8;
                    // Blocks skipped so far (j == bits ie blocks)...
                    result += j;
                    // Return block.
                    return result;
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
        // Any free inodes here?
        GroupDesc *pDesc = m_pGroupDescriptors[group];
        if (!pDesc->bg_free_inodes_count)
        {
            // No inodes free in this group.
            continue;
        }

        // Make sure this block group's inode bitmap has been loaded.
        ensureFreeInodeBitmapLoaded(group);

        // 8 inodes per byte - i == bitmap offset in bytes.
        Vector<size_t> &list = m_pInodeBitmaps[group];
        for (size_t i = 0;
             i < LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group) / 8;
             i += sizeof(uint32_t))
        {
            // Calculate block index into the bitmap.
            size_t idx = i / (m_BlockSize * 8);
            size_t off = i % (m_BlockSize * 8);

            // Grab the specific block for the bitmap.
            /// \todo Endianness - to ensure correct operation, must ptr be
            /// little endian?
            uintptr_t block = list[idx];
            uint32_t *ptr = reinterpret_cast<uint32_t*> (block + off);
            uint32_t tmp = *ptr;

            // If all bits set, avoid searching the bitmap.
            if (tmp == ~0U)
                continue;

            // Check each bit for free inode.
            for (size_t j = 0; j < 32; j++, tmp >>= 1)
            {
                // Free?
                if ((tmp & 1) == 0)
                {
                    // This inode is free! Mark used.
                    *ptr |= (1 << j);
                    pDesc->bg_free_inodes_count--;

                    // Update superblock.
                    m_pSuperblock->s_free_inodes_count--;
                    m_pDisk->write(1024ULL);

                    // Update bitmap on disk.
                    uint32_t desc_block = LITTLE_TO_HOST32(m_pGroupDescriptors[group]->bg_inode_bitmap) + idx;
                    writeBlock(desc_block);

                    // Update group descriptor count on disk.
                    /// \todo save group descriptor block number elsewhere
                    uint32_t gdBlock = LITTLE_TO_HOST32(m_pSuperblock->s_first_data_block) + 1;
                    uint32_t result = (group * sizeof(GroupDesc)) / m_BlockSize;
                    writeBlock(gdBlock + result);

                    // First inode of this group...
                    uint32_t inode = group * LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group);
                    // Inodes skipped so far (i == offset in bytes)...
                    inode += i * 8;
                    // Inodes skipped so far (j == bits ie inodes)...
                    // Note: inodes start counting at one, not zero.
                    inode += j + 1;
                    // Return inode.
                    return inode;
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
    NOTICE("release block: " << Dec << block << Hex);

    // In some ext2 filesystems, this is zero so we don't need to do this. But
    // for those that do, not doing this messes up the bit offsets below.
    block -= LITTLE_TO_HOST32(m_pSuperblock->s_first_data_block);

    uint32_t blocksPerGroup = LITTLE_TO_HOST32(m_pSuperblock->s_blocks_per_group);
    uint32_t group = block / blocksPerGroup;
    uint32_t index = block % blocksPerGroup;

    if (!block)
    {
        // Error out, zero is used as a sentinel in a few places - and we almost
        // certainly never actually mean to free block zero.
        FATAL("Releasing block zero!");
    }

    ensureFreeBlockBitmapLoaded(group);

    // Free block.
    GroupDesc *pDesc = m_pGroupDescriptors[group];

    // Index = block offset from the start of this block.
    size_t bitmapField = (index / 8) / m_BlockSize;
    size_t bitmapOffset = (index / 8) % m_BlockSize;

    Vector<size_t> &list = m_pBlockBitmaps[group];
    uintptr_t diskBlock = list[bitmapField];
    uint8_t *ptr = reinterpret_cast<uint8_t*> (diskBlock + bitmapOffset);
    uint8_t bit = (index % 8);
    if ((*ptr & (1 << bit)) == 0)
        ERROR("bit already freed for block " << Dec << block << Hex);
    *ptr &= ~(1 << bit);

    // Update hints.
    pDesc->bg_free_blocks_count++;
    m_pSuperblock->s_free_blocks_count++;

    // Update superblock.
    m_pDisk->write(1024ULL);

    // Update bitmap on disk.
    uint32_t desc_block = LITTLE_TO_HOST32(m_pGroupDescriptors[group]->bg_block_bitmap) + bitmapField;
    writeBlock(desc_block);

    // Update group descriptor on disk.
    /// \todo save group descriptor block number elsewhere
    uint32_t gdBlock = LITTLE_TO_HOST32(m_pSuperblock->s_first_data_block) + 1;
    uint32_t groupBlock = (group * sizeof(GroupDesc)) / m_BlockSize;
    writeBlock(gdBlock + groupBlock);
}

bool Ext2Filesystem::releaseInode(uint32_t inode)
{
    Inode *pInode = getInode(inode);
    --inode; // Inode zero is undefined, so it's not used.

    uint32_t inodesPerGroup = LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group);
    uint32_t group = inode / inodesPerGroup;
    uint32_t index = inode % inodesPerGroup;

    bool bRemove = decreaseInodeRefcount(inode + 1);

    // Do we need to free this inode?
    if (bRemove)
    {
        // Set dtime on inode.
        pInode->i_dtime = HOST_TO_LITTLE32(getUnixTimestamp());

        ensureFreeInodeBitmapLoaded(group);

        // Free inode.
        GroupDesc *pDesc = m_pGroupDescriptors[group];
        pDesc->bg_free_inodes_count++;
        m_pSuperblock->s_free_inodes_count++;

        // Index = inode offset from the start of this block.
        size_t bitmapField = (index / 8) / m_BlockSize;
        size_t bitmapOffset = (index / 8) % m_BlockSize;

        Vector<size_t> &list = m_pInodeBitmaps[group];
        uintptr_t block = list[bitmapField];
        uint8_t *ptr = reinterpret_cast<uint8_t*> (block + bitmapOffset);
        *ptr &= ~(1 << (index % 8));

        // Update superblock.
        m_pDisk->write(1024ULL);

        // Update on disk.
        uint32_t desc_block = LITTLE_TO_HOST32(m_pGroupDescriptors[group]->bg_inode_bitmap) + bitmapField;
        writeBlock(desc_block);

        // Update group descriptor on disk.
        /// \todo save group descriptor block number elsewhere
        uint32_t gdBlock = LITTLE_TO_HOST32(m_pSuperblock->s_first_data_block) + 1;
        uint32_t groupBlock = (group * sizeof(GroupDesc)) / m_BlockSize;
        writeBlock(gdBlock + groupBlock);
    }

    writeInode(inode);
    return bRemove;
}

Inode *Ext2Filesystem::getInode(uint32_t inode)
{
    inode--; // Inode zero is undefined, so it's not used.

    uint32_t inodesPerGroup = LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group);
    uint32_t group = inode / inodesPerGroup;
    uint32_t index = inode % inodesPerGroup;

    ensureInodeTableLoaded(group);
    Vector<size_t> &list = m_pInodeTables[group];

    size_t blockNum = (index * m_InodeSize) / m_BlockSize;
    size_t blockOff = (index * m_InodeSize) % m_BlockSize;

    uintptr_t block = list[blockNum];

    Inode *pInode = reinterpret_cast<Inode*> (block+blockOff);
    if (pInode->i_flags & EXT2_COMPRBLK_FL)
    {
        WARNING("Ext2: inode " << inode << " has compressed blocks - not yet supported!");
    }
    return pInode;
}

void Ext2Filesystem::writeInode(uint32_t inode)
{
    inode--; // Inode zero is undefined, so it's not used.

    uint32_t inodesPerGroup = LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group);
    uint32_t group = inode / inodesPerGroup;
    uint32_t index = inode % inodesPerGroup;

    ensureInodeTableLoaded(group);

    size_t blockNum = (index * m_InodeSize) / m_BlockSize;
    uint64_t diskBlock = LITTLE_TO_HOST32(m_pGroupDescriptors[group]->bg_inode_table) + blockNum;
    writeBlock(diskBlock);
}

bool Ext2Filesystem::checkOptionalFeature(size_t feature)
{
    if (LITTLE_TO_HOST32(m_pSuperblock->s_rev_level) < 1)
        return false;
    return m_pSuperblock->s_feature_compat & feature;
}

bool Ext2Filesystem::checkRequiredFeature(size_t feature)
{
    if (LITTLE_TO_HOST32(m_pSuperblock->s_rev_level) < 1)
        return false;
    return m_pSuperblock->s_feature_incompat & feature;
}

bool Ext2Filesystem::checkReadOnlyFeature(size_t feature)
{
    if (LITTLE_TO_HOST32(m_pSuperblock->s_rev_level) < 1)
        return false;
    return m_pSuperblock->s_feature_ro_compat & feature;
}


void Ext2Filesystem::ensureFreeBlockBitmapLoaded(size_t group)
{
    assert(group < m_nGroupDescriptors);
    Vector<size_t> &list = m_pBlockBitmaps[group];

    if (list.size() > 0)
        // Descriptors already loaded.
        return;

    // Determine how many blocks to load to bring in the full block bitmap.
    // The bitmap works so that 8 blocks fit into one byte.
    uint32_t blocksPerGroup = LITTLE_TO_HOST32(m_pSuperblock->s_blocks_per_group);
    size_t nBlocks = blocksPerGroup / (m_BlockSize * 8);
    if (blocksPerGroup % (m_BlockSize * 8))
        nBlocks ++;

    for (size_t i = 0; i < nBlocks; i++)
    {
        uint32_t blockNumber = LITTLE_TO_HOST32(m_pGroupDescriptors[group]->bg_block_bitmap) + i;
        list.pushBack(readBlock(blockNumber));
    }
}

void Ext2Filesystem::ensureFreeInodeBitmapLoaded(size_t group)
{
    assert(group < m_nGroupDescriptors);
    Vector<size_t> &list = m_pInodeBitmaps[group];

    if (list.size() > 0)
        // Descriptors already loaded.
        return;

    // Determine how many blocks to load to bring in the full inode bitmap.
    // The bitmap works so that 8 inodes fit into one byte.
    uint32_t inodesPerGroup = LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group);
    size_t nBlocks = inodesPerGroup / (m_BlockSize * 8);
    if (inodesPerGroup % (m_BlockSize * 8))
        nBlocks ++;

    for (size_t i = 0; i < nBlocks; i++)
    {
        uint32_t blockNumber = LITTLE_TO_HOST32(m_pGroupDescriptors[group]->bg_inode_bitmap) + i;
        list.pushBack(readBlock(blockNumber));
    }
}

void Ext2Filesystem::ensureInodeTableLoaded(size_t group)
{
    assert(group < m_nGroupDescriptors);
    Vector<size_t> &list = m_pInodeTables[group];

    if (list.size() > 0)
        // Descriptors already loaded.
        return;

    // Determine how many blocks to load to bring in the full inode table.
    uint32_t inodesPerGroup = LITTLE_TO_HOST32(m_pSuperblock->s_inodes_per_group);
    size_t nBlocks = (inodesPerGroup * m_InodeSize) / m_BlockSize;
    if ((inodesPerGroup * m_InodeSize) / m_BlockSize)
        nBlocks ++;

    // Load each block in the inode table.
    for (size_t i = 0; i < nBlocks; i++)
    {
        uint32_t blockNumber = LITTLE_TO_HOST32(m_pGroupDescriptors[group]->bg_inode_table) + i;
        uintptr_t buffer = readBlock(blockNumber);
        // Avoid callbacks allowing the  wipeout of our inode.
        pinBlock(blockNumber);
        list.pushBack(buffer);
    }
}

void Ext2Filesystem::increaseInodeRefcount(uint32_t inode)
{
    Inode *pInode = getInode(inode);
    if (!pInode)
        return;

    uint32_t current_count = LITTLE_TO_HOST32(pInode->i_links_count);
    pInode->i_links_count = HOST_TO_LITTLE32(current_count + 1);

    writeInode(inode);
}

bool Ext2Filesystem::decreaseInodeRefcount(uint32_t inode)
{
    Inode *pInode = getInode(inode);
    if (!pInode)
        return true;  // No inode found - but didn't decrement to zero.

    uint32_t current_count = LITTLE_TO_HOST32(pInode->i_links_count);
    bool bRemove = current_count <= 1;
    if (current_count)
        pInode->i_links_count = HOST_TO_LITTLE32(current_count - 1);

    writeInode(inode);
    return bRemove;
}

#ifndef EXT2_STANDALONE
static bool initExt2()
{
    VFS::instance().addProbeCallback(&Ext2Filesystem::probe);
    return true;
}

static void destroyExt2()
{
}

MODULE_INFO("ext2", &initExt2, &destroyExt2, "vfs");
#endif

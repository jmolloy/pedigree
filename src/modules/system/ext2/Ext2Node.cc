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

#include "Ext2Node.h"
#include "Ext2Filesystem.h"
#include <utilities/assert.h>
#include <syscallError.h>

Ext2Node::Ext2Node(uintptr_t inode_num, Inode *pInode, Ext2Filesystem *pFs) :
    m_pInode(pInode), m_InodeNumber(inode_num), m_pExt2Fs(pFs), m_pBlocks(0),
    m_nBlocks(0), m_nSize(LITTLE_TO_HOST32(pInode->i_size))
{
    // i_blocks == # of 512-byte blocks. Convert to FS block count.
    uint32_t blockCount = LITTLE_TO_HOST32(pInode->i_blocks);
    m_nBlocks = (blockCount * 512) / m_pExt2Fs->m_BlockSize;

    if (m_nBlocks)
    {
        m_pBlocks = new uint32_t[m_nBlocks];
        memset(m_pBlocks, ~0, sizeof(uint32_t) * m_nBlocks);
    }

    for (size_t i = 0; i < 12 && i < m_nBlocks; i++)
        m_pBlocks[i] = LITTLE_TO_HOST32(m_pInode->i_block[i]);
}

Ext2Node::~Ext2Node()
{
    delete [] m_pBlocks;
}

uintptr_t Ext2Node::readBlock(uint64_t location)
{
    // Sanity check.
    uint32_t nBlock = location / m_pExt2Fs->m_BlockSize;
    if (nBlock > m_nBlocks)
        return 0;
    if (location > m_nSize)
        return 0;

    ensureBlockLoaded(nBlock);
    uintptr_t result = m_pExt2Fs->readBlock(m_pBlocks[nBlock]);

    // Add any remaining offset we chopped off.
    result += location % m_pExt2Fs->m_BlockSize;
    return result;
}

void Ext2Node::writeBlock(uint64_t location)
{
    // Sanity check.
    uint32_t nBlock = location / m_pExt2Fs->m_BlockSize;
    if (nBlock > m_nBlocks)
        return;
    if (location > m_nSize)
        return;

    // Update on disk.
    ensureBlockLoaded(nBlock);
    m_pExt2Fs->writeBlock(m_pBlocks[nBlock]);
}

void Ext2Node::trackBlock(uint32_t block)
{
    /// \todo consider a Vector here instead of doing this by hand
    uint32_t *pTmp = new uint32_t[m_nBlocks + 1];
    memcpy(pTmp, m_pBlocks, m_nBlocks * sizeof(uint32_t));

    delete [] m_pBlocks;
    m_pBlocks = pTmp;

    m_pBlocks[m_nBlocks++] = block;

    // Inode i_blocks field is actually the count of 512-byte blocks.
    uint32_t i_blocks = (m_nBlocks * m_pExt2Fs->m_BlockSize) / 512;
    m_pInode->i_blocks = HOST_TO_LITTLE32(i_blocks);

    // Write updated inode.
    m_pExt2Fs->writeInode(getInodeNumber());
}

void Ext2Node::wipe()
{
    NOTICE("wipe: " << m_nBlocks << " blocks, size is " << m_nSize << "...");
    for (size_t i = 0; i < m_nBlocks; i++)
    {
        ensureBlockLoaded(i);
        NOTICE("wipe: releasing block");
        m_pExt2Fs->releaseBlock(m_pBlocks[i]);
    }

    m_nSize = 0;
    m_nBlocks = 0;

    m_pInode->i_size = 0;
    m_pInode->i_blocks = 0;
    memset(m_pInode->i_block, 0, sizeof(uint32_t) * 15);

    // Write updated inode.
    m_pExt2Fs->writeInode(getInodeNumber());
    NOTICE("wipe done");
}

void Ext2Node::extend(size_t newSize)
{
    ensureLargeEnough(newSize);
}

bool Ext2Node::ensureLargeEnough(size_t size)
{
    if (size > m_nSize)
    {
        m_nSize = size;
        fileAttributeChanged(m_nSize, LITTLE_TO_HOST32(m_pInode->i_atime),
                             LITTLE_TO_HOST32(m_pInode->i_mtime),
                             LITTLE_TO_HOST32(m_pInode->i_ctime));
    }

    while (size > m_nBlocks*m_pExt2Fs->m_BlockSize)
    {
        uint32_t block = m_pExt2Fs->findFreeBlock(m_InodeNumber);
        if (block == 0)
        {
            // We had a problem.
            SYSCALL_ERROR(NoSpaceLeftOnDevice);
            return false;
        }
        if (!addBlock(block))
        {
            ERROR("Adding block " << block << " failed!");
            return false;
        }
        // Load the block and zero it.
        uint8_t *pBuffer = reinterpret_cast<uint8_t*>(m_pExt2Fs->readBlock(block));
        memset(pBuffer, 0, m_pExt2Fs->m_BlockSize);
    }
    return true;
}

bool Ext2Node::ensureBlockLoaded(size_t nBlock)
{
    if (nBlock >= m_nBlocks)
    {
        FATAL("EXT2: ensureBlockLoaded: Algorithmic error.");
    }
    if (m_pBlocks[nBlock] == ~0U)
        getBlockNumber(nBlock);

    return true;
}

bool Ext2Node::getBlockNumber(size_t nBlock)
{
    size_t nPerBlock = m_pExt2Fs->m_BlockSize/4;

    assert (nBlock >= 12);

    if (nBlock < nPerBlock+12)
    {
        getBlockNumberIndirect(LITTLE_TO_HOST32(m_pInode->i_block[12]), 12, nBlock);
        return true;
    }

    if (nBlock < (nPerBlock*nPerBlock)+nPerBlock+12)
    {
        getBlockNumberBiindirect(LITTLE_TO_HOST32(m_pInode->i_block[13]), nPerBlock+12, nBlock);
        return true;
    }

    getBlockNumberTriindirect(LITTLE_TO_HOST32(m_pInode->i_block[14]),
                              (nPerBlock*nPerBlock)+nPerBlock+12, nBlock);

    return true;
}

bool Ext2Node::getBlockNumberIndirect(uint32_t inode_block, size_t nBlocks, size_t nBlock)
{
    uint32_t *buffer = reinterpret_cast<uint32_t*>(m_pExt2Fs->readBlock(inode_block));

    for (size_t i = 0; i < m_pExt2Fs->m_BlockSize/4 && nBlocks < m_nBlocks; i++)
    {
        m_pBlocks[nBlocks++] = LITTLE_TO_HOST32(buffer[i]);
    }

    return true;
}

bool Ext2Node::getBlockNumberBiindirect(uint32_t inode_block, size_t nBlocks, size_t nBlock)
{
    size_t nPerBlock = m_pExt2Fs->m_BlockSize/4;

    uint32_t *buffer = reinterpret_cast<uint32_t*>(m_pExt2Fs->readBlock(inode_block));

    // What indirect block does nBlock exist on?
    size_t nIndirectBlock = (nBlock-nBlocks) / nPerBlock;

    getBlockNumberIndirect(LITTLE_TO_HOST32(buffer[nIndirectBlock]),
                           nBlocks+nIndirectBlock*nPerBlock, nBlock);

    return true;
}

bool Ext2Node::getBlockNumberTriindirect(uint32_t inode_block, size_t nBlocks, size_t nBlock)
{
    size_t nPerBlock = m_pExt2Fs->m_BlockSize/4;

    uint32_t *buffer = reinterpret_cast<uint32_t*>(m_pExt2Fs->readBlock(inode_block));

    // What biindirect block does nBlock exist on?
    size_t nBiBlock = (nBlock-nBlocks) / (nPerBlock*nPerBlock);

    getBlockNumberBiindirect(LITTLE_TO_HOST32(buffer[nBiBlock]),
                             nBlocks+nBiBlock*nPerBlock*nPerBlock, nBlock);

    return true;
}

bool Ext2Node::addBlock(uint32_t blockValue)
{
    size_t nEntriesPerBlock = m_pExt2Fs->m_BlockSize/4;

    // Calculate whether direct, indirect or tri-indirect addressing is needed.
    if (m_nBlocks < 12)
    {
        // Direct addressing is possible.
        m_pInode->i_block[m_nBlocks] = HOST_TO_LITTLE32(blockValue);
    }
    else if (m_nBlocks < 12 + nEntriesPerBlock)
    {
        // Indirect addressing needed.
        size_t indirectIdx = m_nBlocks - 12;

        // If this is the first indirect block, we need to reserve a new table block.
        if (m_nBlocks == 12)
        {
            uint32_t newBlock = m_pExt2Fs->findFreeBlock(m_InodeNumber);
            m_pInode->i_block[12] = HOST_TO_LITTLE32(newBlock);
            if (m_pInode->i_block[12] == 0)
            {
                // We had a problem.
                SYSCALL_ERROR(NoSpaceLeftOnDevice);
                return false;
            }

            void *buffer = reinterpret_cast<void *>(m_pExt2Fs->readBlock(newBlock));
            memset(buffer, 0, m_pExt2Fs->m_BlockSize);

            // Taken on a new block - update block count (but don't track in
            // m_pBlocks, as this is a metadata block).
            m_nBlocks++;
        }

        // Now we can set the block.
        uint32_t bufferBlock = LITTLE_TO_HOST32(m_pInode->i_block[12]);
        uint32_t *buffer = reinterpret_cast<uint32_t*>(m_pExt2Fs->readBlock(bufferBlock));

        buffer[indirectIdx] = HOST_TO_LITTLE32(blockValue);
        m_pExt2Fs->writeBlock(bufferBlock);
    }
    else if (m_nBlocks < 12 + nEntriesPerBlock + nEntriesPerBlock*nEntriesPerBlock)
    {
        // Bi-indirect addressing required.

        // Index from the start of the bi-indirect block (i.e. ignore the 12 direct entries and one indirect block).
        size_t biIdx = m_nBlocks - 12 - nEntriesPerBlock;
        // Block number inside the bi-indirect table of where to find the indirect block table.
        size_t indirectBlock = biIdx / nEntriesPerBlock;
        // Index inside the indirect block table.
        size_t indirectIdx = biIdx % nEntriesPerBlock;

        // If this is the first bi-indirect block, we need to reserve a bi-indirect table block.
        if (biIdx == 0)
        {
            uint32_t newBlock = m_pExt2Fs->findFreeBlock(m_InodeNumber);
            m_pInode->i_block[13] = HOST_TO_LITTLE32(newBlock);
            if (m_pInode->i_block[13] == 0)
            {
                // We had a problem.
                SYSCALL_ERROR(NoSpaceLeftOnDevice);
                return false;
            }

            void *buffer = reinterpret_cast<void *>(m_pExt2Fs->readBlock(newBlock));
            memset(buffer, 0, m_pExt2Fs->m_BlockSize);

            // Taken on a new block - update block count (but don't track in
            // m_pBlocks, as this is a metadata block).
            m_nBlocks++;
        }

        // Now we can safely read the bi-indirect block.
        uint32_t bufferBlock = LITTLE_TO_HOST32(m_pInode->i_block[13]);
        uint32_t *pBlock = reinterpret_cast<uint32_t*>(m_pExt2Fs->readBlock(bufferBlock));

        // Do we need to start a new indirect block?
        if (indirectIdx == 0)
        {
            uint32_t newBlock = m_pExt2Fs->findFreeBlock(m_InodeNumber);
            pBlock[indirectBlock] = HOST_TO_LITTLE32(newBlock);
            if (pBlock[indirectBlock] == 0)
            {
                // We had a problem.
                SYSCALL_ERROR(NoSpaceLeftOnDevice);
                return false;
            }

            m_pExt2Fs->writeBlock(bufferBlock);

            void *buffer = reinterpret_cast<void *>(m_pExt2Fs->readBlock(newBlock));
            memset(buffer, 0, m_pExt2Fs->m_BlockSize);

            // Taken on a new block - update block count (but don't track in
            // m_pBlocks, as this is a metadata block).
            m_nBlocks++;
        }

        // Cache this as it gets clobbered by the readBlock call (using the same buffer).
        uint32_t nIndirectBlockNum = LITTLE_TO_HOST32(pBlock[indirectBlock]);

        // Grab the indirect block.
        pBlock = reinterpret_cast<uint32_t*>(m_pExt2Fs->readBlock(nIndirectBlockNum));

        // Set the correct entry.
        pBlock[indirectIdx] = HOST_TO_LITTLE32(blockValue);
        m_pExt2Fs->writeBlock(nIndirectBlockNum);
    }
    else
    {
        // Tri-indirect addressing required.
        FATAL("EXT2: Tri-indirect addressing required, but not implemented.");
        return false;
    }

    trackBlock(blockValue);

    return true;
}

void Ext2Node::fileAttributeChanged(size_t size, size_t atime, size_t mtime, size_t ctime)
{
    // Reconstruct the inode from the cached fields.
    uint32_t i_blocks = (m_nBlocks * m_pExt2Fs->m_BlockSize) / 512;
    m_pInode->i_blocks = HOST_TO_LITTLE32(i_blocks);
    m_pInode->i_size = HOST_TO_LITTLE32(size); /// \todo 4GB files.
    m_pInode->i_atime = HOST_TO_LITTLE32(atime);
    m_pInode->i_mtime = HOST_TO_LITTLE32(mtime);
    m_pInode->i_ctime = HOST_TO_LITTLE32(ctime);

    // Update our internal record of the file size accordingly.
    m_nSize = size;

    // Write updated inode.
    m_pExt2Fs->writeInode(getInodeNumber());
}

void Ext2Node::updateMetadata(uint16_t uid, uint16_t gid, uint32_t perms)
{
    // Avoid wiping out extra mode bits that Pedigree doesn't yet care about.
    uint32_t curr_mode = LITTLE_TO_HOST32(m_pInode->i_mode);
    curr_mode &= ~((1 << 9) - 1);
    curr_mode |= perms;

    m_pInode->i_uid = HOST_TO_LITTLE16(uid);
    m_pInode->i_gid = HOST_TO_LITTLE16(gid);
    m_pInode->i_mode = HOST_TO_LITTLE32(curr_mode);

    // Write updated inode.
    m_pExt2Fs->writeInode(getInodeNumber());
}

void Ext2Node::sync(size_t offset, bool async)
{
    uint32_t nBlock = offset / m_pExt2Fs->m_BlockSize;
    if (nBlock > m_nBlocks)
        return;
    if (offset > m_nSize)
        return;

    // Sync the block.
    ensureBlockLoaded(nBlock);
    m_pExt2Fs->sync(m_pBlocks[nBlock] * m_pExt2Fs->m_BlockSize, async);
}

void Ext2Node::pinBlock(uint64_t location)
{
    uint32_t nBlock = location / m_pExt2Fs->m_BlockSize;
    if (nBlock > m_nBlocks)
        return;
    if (location > m_nSize)
        return;

    ensureBlockLoaded(nBlock);
    m_pExt2Fs->pinBlock(m_pBlocks[nBlock]);
}

void Ext2Node::unpinBlock(uint64_t location)
{
    uint32_t nBlock = location / m_pExt2Fs->m_BlockSize;
    if (nBlock > m_nBlocks)
        return;
    if (location > m_nSize)
        return;

    ensureBlockLoaded(nBlock);
    m_pExt2Fs->unpinBlock(m_pBlocks[nBlock]);
}

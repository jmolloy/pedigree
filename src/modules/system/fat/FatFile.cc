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

#include "FatFile.h"
#include "FatFilesystem.h"

#include <process/Mutex.h>
#include <utilities/MemoryPool.h>
#include <LockGuard.h>

#define FAT_BLOCK_POOL_BLKSIZE  (PhysicalMemoryManager::getPageSize())
#define FAT_BLOCK_POOL_SIZE     ((FAT_BLOCK_POOL_BLKSIZE) * 16384)

MemoryPool g_FatBlockRegion("FatFileBlockCache");
Mutex g_FatCacheLock(false);

FatFile::FatFile(String name, Time accessedTime, Time modifiedTime, Time creationTime,
       uintptr_t inode, class Filesystem *pFs, size_t size, uint32_t dirClus, uint32_t dirOffset, File *pParent) :
  File(name,accessedTime,modifiedTime,creationTime,inode,pFs,size,pParent),
  m_DirClus(dirClus), m_DirOffset(dirOffset)
{
    if(!g_FatBlockRegion.initialised())
    {
        g_FatBlockRegion.initialise(FAT_BLOCK_POOL_SIZE / PhysicalMemoryManager::getPageSize(), FAT_BLOCK_POOL_BLKSIZE);
    }
}

FatFile::~FatFile()
{
}

uintptr_t FatFile::readBlock(uint64_t location)
{
#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    // Switch to the kernel address space because of reasons.
    // It seems like things like ATA operate in the kernel address space and
    // do not see our MemoryPool(s) or anything like that. Ouch.
    VirtualAddressSpace &VAddressSpace = Processor::information().getVirtualAddressSpace();
    Processor::switchAddressSpace(VirtualAddressSpace::getKernelAddressSpace());
#endif

    FatFilesystem *pFs = reinterpret_cast<FatFilesystem*>(m_pFilesystem);

    /// \note Not freed. Watch out.
    /// \todo THIS IS TERRIBLE ARE YOU SERIOUS. NOT FREED!?
    /// \todo THIS IS ALSO TERRIBLE BECAUSE 4K * 16K IS THE ENTIRE POOL SIZE
    ///       THAT IS ONLY 64 MB IT SHOULD GROW DYNAMICALLY OR SOMETHING.
    /// \todo PUT THAT Cache THING IN HERE BECAUSE IT MAKES FAR MORE SENSE
    /// \note LOUD NOISES
    g_FatCacheLock.acquire();
    uintptr_t buffer = g_FatBlockRegion.allocate();
    g_FatCacheLock.release();
    pFs->read(this, location, getBlockSize(), buffer);

#ifdef KERNEL_NEEDS_ADDRESS_SPACE_SWITCH
    Processor::switchAddressSpace(VAddressSpace);
#endif

    return buffer;
}

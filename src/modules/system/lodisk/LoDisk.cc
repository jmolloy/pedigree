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

#include "../Module.h"
#include "LoDisk.h"
#include <utilities/assert.h>

#include <ServiceManager.h>

FileDisk::FileDisk(String file, AccessType mode) :
    m_pFile(0), m_Mode(mode), m_PageCache(), m_MemRegion("FileDisk")
{
    m_pFile = VFS::instance().find(file);
    if(!m_pFile)
        WARNING("FileDisk: '" << file << "' doesn't exist...");
    else
    {
        NOTICE("Initialising...");
        m_pFile->increaseRefCount(false);

        // Chat to the partition service and let it pick up that we're around now
        ServiceFeatures *pFeatures = ServiceManager::instance().enumerateOperations(String("partition"));
        Service         *pService  = ServiceManager::instance().getService(String("partition"));
        NOTICE("Asking if the partition provider supports touch");
        if(pFeatures->provides(ServiceFeatures::touch))
        {
            NOTICE("It does, attempting to inform the partitioner of our presence...");
            if(pService)
            {
                if(pService->serve(ServiceFeatures::touch, reinterpret_cast<void*>(this), sizeof(FileDisk)))
                    NOTICE("Successful.");
                else
                    ERROR("Failed.");
            }
            else
                ERROR("FileDisk: Couldn't tell the partition service about the new disk presence");
        }
        else
            ERROR("FileDisk: Partition service doesn't appear to support touch");
    }
}

FileDisk::~FileDisk()
{
    m_pFile->decreaseRefCount(false);
}

bool FileDisk::initialise()
{
    return (m_pFile != 0);
}

uint64_t FileDisk::read(uint64_t location, uint64_t nBytes, uintptr_t buffer)
{
    if(!m_pFile)
        return 0;

    // Determine which page the read is in
    uint64_t readPage = location / FILEDISK_PAGE_SIZE;
    uint64_t pageOffset = location % FILEDISK_PAGE_SIZE;

    // How many pages does the read cross?
    uint64_t pageCount = nBytes / FILEDISK_PAGE_SIZE;
    if(pageOffset)
        pageCount++;
    if((nBytes % FILEDISK_PAGE_SIZE))
        pageCount++;

    // Read the data
    uint8_t *dest = reinterpret_cast<uint8_t *>(buffer);
    uint64_t startPage = readPage, numBytes = nBytes;
    for(uint64_t n = 0; n < pageCount && numBytes > 0; n++)
    {
        // How many bytes are to be read during this round?
        uint64_t nBytesToRead = numBytes > FILEDISK_PAGE_SIZE ? FILEDISK_PAGE_SIZE : numBytes;

        // Look up in the cache
        uint8_t *destPage = dest + (FILEDISK_PAGE_SIZE * n);
        uint8_t *buff = 0;
        if((buff = reinterpret_cast<uint8_t *>(m_PageCache.lookup(startPage + n))))
        {
            // Single copy, rather than multiple
            memcpy(destPage, buff + pageOffset, nBytesToRead);
        }
        else
        {
            // Allocate space for the page cache
            /// \todo Is this correct? Are these pages still freeable, even if the mem region is updated?
            if(!PhysicalMemoryManager::instance().allocateRegion(m_MemRegion, FILEDISK_PAGE_SIZE / PhysicalMemoryManager::instance().getPageSize(), 0, VirtualAddressSpace::Write, -1))
            {
                ERROR("FileDisk: Couldn't allocate space for a block\n");
                return 0;
            }
            buff = reinterpret_cast<uint8_t*>(m_MemRegion.virtualAddress());

            // Read the data from the file itself
            uint64_t sz = m_pFile->read((startPage + n) * FILEDISK_PAGE_SIZE, FILEDISK_PAGE_SIZE, reinterpret_cast<uintptr_t>(buff));

            // Verify the size - it has to fit in with what we asked for
            assert(sz <= FILEDISK_PAGE_SIZE);
            assert(sz > 0);
            
            // Clean up the top of the buffer (just in case we don't read a full page)
            if(FILEDISK_PAGE_SIZE - sz)
                memset(buff + sz, 0, FILEDISK_PAGE_SIZE - sz);
            
            // All is well - copy into the buffer & insert to the page cache
            memcpy(destPage, buff + pageOffset, nBytesToRead);
            m_PageCache.insert(startPage + n, buff);
        }

        // Another page read
        pageOffset = 0;
        numBytes -= nBytesToRead;
    }

    return (nBytes - numBytes);
}

uint64_t FileDisk::write(uint64_t location, uint64_t nBytes, uintptr_t buffer)
{
    if(!m_pFile)
        return 0;

    // Determine which page the read is in
    uint64_t writePage = location / FILEDISK_PAGE_SIZE;
    uint64_t pageOffset = location % FILEDISK_PAGE_SIZE;

    // How many pages does the read cross?
    uint64_t pageCount = nBytes / FILEDISK_PAGE_SIZE;
    if(pageOffset)
        pageCount++;

    // Read the data
    uint8_t *src = reinterpret_cast<uint8_t *>(buffer);
    uint64_t startPage = writePage, numBytes = nBytes;
    for(uint64_t n = 0; n < pageCount && numBytes > 0; n++)
    {
        // How many bytes are to be written during this round?
        uint64_t nBytesToWrite = numBytes > FILEDISK_PAGE_SIZE ? FILEDISK_PAGE_SIZE : numBytes;

        // Look up in the cache
        uint8_t *srcPage = src + (FILEDISK_PAGE_SIZE * n);
        uint8_t *buff = 0;
        buff = reinterpret_cast<uint8_t*>(m_PageCache.lookup(startPage + n));

        if(!buff)
        {
            // Read into the cache...
            uint8_t a = 0;
            read((startPage + n) * FILEDISK_PAGE_SIZE, 1, reinterpret_cast<uintptr_t>(&a));

            // Attempt the lookup again
            buff = reinterpret_cast<uint8_t*>(m_PageCache.lookup(startPage + n));
            assert(buff);
        }

        if(buff)
        {
            // Already have the page in the cache - copy this region
            memcpy(buff + pageOffset, srcPage, nBytesToWrite);

            // If we're not a RAM-only disk, write to the actual File
            if(m_Mode != RamOnly)
                m_pFile->write((startPage + n) * FILEDISK_PAGE_SIZE, FILEDISK_PAGE_SIZE, reinterpret_cast<uintptr_t>(buff));
        }

        // Another page read
        pageOffset = 0;
        numBytes -= nBytesToWrite;
    }

    return (nBytes - numBytes);
}

void init()
{
}

void destroy()
{
}

MODULE_NAME("lodisk");
MODULE_ENTRY(&init);
MODULE_EXIT(&destroy);
MODULE_DEPENDS("VFS");

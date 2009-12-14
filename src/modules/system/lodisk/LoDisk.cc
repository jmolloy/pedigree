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
    m_pFile(0), m_Mode(mode), m_Cache(), m_MemRegion("FileDisk")
{
    m_pFile = VFS::instance().find(file);
    if(!m_pFile)
        WARNING("FileDisk: '" << file << "' doesn't exist...");
    else
    {
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

uintptr_t FileDisk::read(uint64_t location)
{
    if (location % 512)
        FATAL("Read with location % 512.");

    if(!m_pFile)
        return 0;

    // Determine which page the read is in
    uint64_t readPage = location & ~0xFFFUL;
    uint64_t pageOffset = location % 4096;

    uintptr_t buffer = m_Cache.lookup(readPage);

    if (buffer)
        return buffer + pageOffset;

    buffer = m_Cache.insert(readPage);

    // Read the data from the file itself
    uint64_t sz = m_pFile->read(readPage, 4096, buffer);

    return buffer + pageOffset;
}

void FileDisk::write(uint64_t location)
{
    if(!m_pFile)
        return;

    WARNING("FileDisk::write: Not implemented.");
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
MODULE_DEPENDS("vfs");

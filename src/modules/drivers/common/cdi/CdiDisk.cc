/*
* Copyright (c) 2009 Kevin Wolf <kevin@tyndur.org>
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITRTLSS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, RTLGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONRTLCTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#include "CdiDisk.h"

#include <Log.h>
#include <ServiceManager.h>
#include <utilities/assert.h>

#include <vfs/VFS.h>

// Prototypes in the extern "C" block to ensure that they are not mangled
extern "C" {
    void cdi_cpp_disk_register(void* void_pdev, struct cdi_storage_device* device);

    int cdi_storage_read(struct cdi_storage_device* device, uint64_t pos, size_t size, void* dest);
    int cdi_storage_write(struct cdi_storage_device* device, uint64_t pos, size_t size, void* src);
};

CdiDisk::CdiDisk(Disk* pDev, struct cdi_storage_device* device) :
    Disk(pDev), m_Device(device), m_Cache()
{
    setSpecificType(String("CDI Disk"));
}

CdiDisk::~CdiDisk()
{
}

bool CdiDisk::initialise()
{
    // Chat to the partition service and let it pick up that we're around now
    ServiceFeatures *pFeatures = ServiceManager::instance().enumerateOperations(String("partition"));
    Service         *pService  = ServiceManager::instance().getService(String("partition"));
    NOTICE("Asking if the partition provider supports touch");
    if(pFeatures->provides(ServiceFeatures::touch))
    {
        NOTICE("It does, attempting to inform the partitioner of our presence...");
        if(pService)
        {
            if(pService->serve(ServiceFeatures::touch, 
                               reinterpret_cast<void*>(static_cast<Disk*>(this)), 
                               sizeof(*static_cast<Disk*>(this))))
            {
                NOTICE("Successful.");
            }
            else
            {
                ERROR("Failed.");
                return false;
            }
        }
        else
        {
            ERROR("FileDisk: Couldn't tell the partition service about the new disk presence");
            return false;
        }
    }
    else
    {
        ERROR("FileDisk: Partition service doesn't appear to support touch");
        return false;
    }

    return true;
}

// These are the functions that others call - they add a request to the parent controller's queue.
uintptr_t CdiDisk::read(uint64_t location)
{
    assert( (location % 512) == 0 );
    uintptr_t buff = m_Cache.lookup(location);
    if (!buff)
    {
        buff = m_Cache.insert(location);
        if (cdi_storage_read(m_Device, location, 512, reinterpret_cast<void*>(buff)) != 0)
            return 0;
    }
    return buff;
}

void CdiDisk::write(uint64_t location)
{
    assert( (location % 512) == 0 );
    uintptr_t buff = m_Cache.lookup(location);
    assert(buff);

    if (cdi_storage_write(m_Device, location, 512, reinterpret_cast<void*>(buff)) != 0)
        return;
}

void cdi_cpp_disk_register(void* void_pdev, struct cdi_storage_device* device)
{
    Disk* pDev = reinterpret_cast<Disk*>(void_pdev);

    // Create a new CdiDisk node.
    /// \todo I'm assuming pDev is invalid at this stage. This isn't necessarily
    ///       correct, but it's a quick fix that should work for now. It will need
    ///       to be fixed later!
    CdiDisk *pCdiDisk = new CdiDisk(pDev, device);
    if(!pCdiDisk->initialise())
    {
        delete pCdiDisk;
        return;
    }
    device->dev.backdev = reinterpret_cast<void*>(pCdiDisk);

    // Insert into the tree, properly
    pCdiDisk->setParent(&Device::root());
    Device::root().addChild(pCdiDisk);
}

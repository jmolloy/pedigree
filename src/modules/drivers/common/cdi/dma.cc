/*
 * Copyright (c) 2007 Antoine Kaufmann
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it 
 * and/or modify it under the terms of the Do What The Fuck You Want 
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */  

#include <processor/types.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/VirtualAddressSpace.h>
#include <Log.h>

#include <stdio.h>
#include <stdlib.h>

#include "cdi/dma.h"

#include <dma/IsaDma.h>

#ifndef X86_COMMON
#warning ISA DMA not supported on non-x86 architectures. TODO: FIXME
#endif

/**
 * Initialisiert einen Transport per DMA
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_dma_open(struct cdi_dma_handle* handle, uint8_t channel, uint8_t mode, size_t length, void* buffer)
{
#ifdef X86_COMMON
    // All good
    handle->channel = channel;
    handle->mode = mode;
    handle->length = length;
    handle->buffer = handle->meta.realbuffer = buffer;

    MemoryRegion* region = new MemoryRegion("isa-dma");
    size_t page_size = PhysicalMemoryManager::instance().getPageSize();

    // Allocate memory under 16 MB for the transfer
    if (!PhysicalMemoryManager::instance().allocateRegion(*region,
                                                          (length + page_size - 1) / page_size,
                                                          PhysicalMemoryManager::continuous | PhysicalMemoryManager::below16MB,
                                                          VirtualAddressSpace::Write,
                                                          -1))
    {
        WARNING("cdi: Couldn't allocate physical memory for DMA!");
        delete region;
        return -1;
    }

    // Add the region to the handle
    handle->meta.region = reinterpret_cast<void*>(region);
    handle->buffer = reinterpret_cast<void*>(region->virtualAddress());
    memset(handle->buffer, 0, handle->length);

    // Do the deed
    if(IsaDma::instance().initTransfer(handle->channel, handle->mode, handle->length, region->physicalAddress()))
        return 0;
    else
    {
        delete region;
        return -1;
    }
#else
    return -1; /// \todo Other architectures
#endif
}

/**
 * Liest Daten per DMA ein
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_dma_read(struct cdi_dma_handle* handle)
{
#ifdef X86_COMMON
    // Copy the memory across
    memcpy(handle->meta.realbuffer, handle->buffer, handle->length);
    return 0;
#else
    return -1;
#endif
}

/**
 * Schreibt Daten per DMA
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_dma_write(struct cdi_dma_handle* handle)
{
#ifdef X86_COMMON
    // Copy the memory across
    memcpy(handle->buffer, handle->meta.realbuffer, handle->length);
    return 0;
#else
    return -1;
#endif
}

/**
 * Schliesst das DMA-Handle wieder
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int cdi_dma_close(struct cdi_dma_handle* handle)
{
#ifdef X86_COMMON
    // Grab the region from the handle and free it
    MemoryRegion* region = reinterpret_cast<MemoryRegion*>(handle->meta.region);
    delete region;

    return 0;
#else
    return -1;
#endif
}

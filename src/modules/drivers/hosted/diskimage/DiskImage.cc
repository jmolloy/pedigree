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

#include <BootstrapInfo.h>
#include <Log.h>
#include "DiskImage.h"

extern BootstrapStruct_t *g_pBootstrapInfo;

bool DiskImage::initialise()
{
    if(g_pBootstrapInfo->getModuleCount() < 3)
    {
        NOTICE("not enough modules to create a DiskImage");
        return false;
    }

    uintptr_t baseAddress = *reinterpret_cast<uintptr_t*>(adjust_pointer(g_pBootstrapInfo->getModuleBase(), 32));
    uintptr_t endAddress = *reinterpret_cast<uintptr_t*>(adjust_pointer(g_pBootstrapInfo->getModuleBase(), 40));

    m_pBase = reinterpret_cast<void *>(baseAddress);
    m_nSize = endAddress - baseAddress;
    return true;
}

uintptr_t DiskImage::read(uint64_t location)
{
    if(location > m_nSize)
    {
        return 0;
    }

    if(m_pBase)
    {
        return reinterpret_cast<uintptr_t>(adjust_pointer(m_pBase, location));
    }

    return 0;
}

size_t DiskImage::getSize() const
{
    return m_nSize;
}

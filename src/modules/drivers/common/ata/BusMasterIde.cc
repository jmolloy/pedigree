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
#include "BusMasterIde.h"
#include <processor/types.h>
#include <processor/IoBase.h>
#include <processor/IoPort.h>
#include <processor/Processor.h>
#include <processor/MemoryRegion.h>
#include <processor/PhysicalMemoryManager.h>
#include <LockGuard.h>

BusMasterIde::BusMasterIde() :
    m_pBase(0), m_PrdTableLock(false), m_PrdTable(0), m_LastPrdTableOffset(0),
    m_PrdTablePhys(0), m_PrdTableMemRegion("bus-master-ide")
{
}

BusMasterIde::~BusMasterIde()
{
    if(m_pBase)
    {
        delete m_pBase;
        m_pBase = 0;
    }
}

bool BusMasterIde::initialise(IoBase *pBase)
{
    // Firstly verify the incoming ports
    if(!pBase)
        return false;
    // Deviation from convention a bit here... The controller creating the
    // IoBase for us to use will split the port range into Primary/Secondary
    // ranges, which simplifies driver code.
    if(pBase->size() != 6)
        return false;

    // Right, it seems the I/O base checks out... Try and allocate some memory
    // for the PRDT now.
    if(!PhysicalMemoryManager::instance().allocateRegion(m_PrdTableMemRegion,
                                                       1,
                                                       PhysicalMemoryManager::continuous,
                                                       VirtualAddressSpace::Write,
                                                       -1))
    {
        ERROR("BusMasterIde: Couldn't allocate the PRD table!");
        return false;
    }
    else
    {
        m_PrdTable = reinterpret_cast<PhysicalRegionDescriptor *>(m_PrdTableMemRegion.virtualAddress());
        m_PrdTablePhys = m_PrdTableMemRegion.physicalAddress();
        NOTICE("BusMasterIde: PRD table at v=" << reinterpret_cast<uintptr_t>(m_PrdTableMemRegion.virtualAddress()) << ", p=" << m_PrdTablePhys << ".");
    }

    // All is well, set our internal port base and return success!
    m_pBase = pBase;
    return true;
}

bool BusMasterIde::begin(uintptr_t buffer, size_t nBytes, bool bWrite)
{
    // Sanity check
    if(!buffer || !nBytes || !m_pBase)
        return false;

    // A couple of useful things to know
    size_t prdTableEntries = (m_PrdTableMemRegion.size() / sizeof(PhysicalRegionDescriptor));

    // Firstly check if we need to wait a bit... If an operation
    uint8_t statusReg = m_pBase->read8(Status);
    /// \todo Can't write if a read is in progress, and vice versa

    // Okay, we're good to go - check that the buffer is mapped in first
    LockGuard<Mutex> guard(m_PrdTableLock);
    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();
    if(va.isMapped(reinterpret_cast<void*>(buffer)))
    {
        // Okay, the address is mapped, so we can start inserting PRD table
        // entries.
        size_t nRemainingBytes = nBytes, currOffset = 0, i = 0;
        while(nRemainingBytes && ((m_LastPrdTableOffset + i) < prdTableEntries))
        {
            // Grab the mapping of this specfic portion of the block. Each PRD
            // entry covers a 4096-byte region of memory (it can cover up to
            // 65,536 bytes, but by using only 4096-byte regions it is possible
            // to avoid the contiguous physical RAM requirement).
            physical_uintptr_t physPage = 0; size_t flags = 0;
            va.getMapping(reinterpret_cast<void*>(buffer), physPage, flags);

            // Add in whatever offset into the page we may have in the buffer
            // parameter.
            size_t pageOffset = 0;
            if(i == 0)
                pageOffset = (buffer & 0xFFF);

            // Install into the PRD table now
            m_PrdTable[m_LastPrdTableOffset + i].physAddr = physPage + pageOffset;

            // Determine the transfer size we should use
            size_t transferSize = nRemainingBytes;
            if(transferSize > 4096)
                transferSize = 4096 - pageOffset;
            m_PrdTable[m_LastPrdTableOffset + i].byteCount = transferSize & 0xFFFF;

            // Complete the PRD entry after determining the next offset
            currOffset += transferSize;
            nRemainingBytes -= transferSize;
            if(!nRemainingBytes)
                m_PrdTable[m_LastPrdTableOffset + i].rsvdEot = 0x8000; // End-of-table?
            else
                m_PrdTable[m_LastPrdTableOffset + i].rsvdEot = 0;
            i++;
        }

        // If we added an entry, remove the EOT from any previous PRD that was
        // present.
        if(i && m_LastPrdTableOffset)
            m_PrdTable[m_LastPrdTableOffset - 1].rsvdEot = 0;
        // If we didn't add an entry, just return failure
        else if(!i)
            return false;

        // If no other command is running, set the PRD physical address and
        // begin the command.
        statusReg = m_pBase->read8(Status);
        if(!(statusReg & 0x1))
        {
            // Write the physical address to the table address register
            m_pBase->write32(m_PrdTablePhys, PrdTableAddr);

            // Begin the command
            uint8_t cmdReg = m_pBase->read8(Command);
            if(cmdReg & 0x1)
                FATAL("BusMaster IDE status and command registers don't make sense");
            cmdReg = (cmdReg & 0xF6) | 0x1 | (bWrite ? 8 : 0);
            m_pBase->write8(cmdReg, BusMasterIde::Command);
        }
    }
    else
        return false; // Buffer not mapped - nothing we can do!
}

bool BusMasterIde::hasInterrupt()
{
    // Sanity check
    if(!m_pBase)
        return false;

    // Easy check here
    return (m_pBase->read8(Status) & 0x4);
}

bool BusMasterIde::hasError()
{
    // Sanity check
    if(!m_pBase)
        return false;

    // Easy check here
    return (m_pBase->read8(Status) & 0x1);
}

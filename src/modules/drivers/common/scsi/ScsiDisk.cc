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

#include <processor/types.h>
#include <utilities/assert.h>
#include <utilities/Cache.h>
#include <ServiceManager.h>
#include "ScsiDisk.h"
#include "ScsiCommands.h"
#include "ScsiController.h"
#include <utilities/PointerGuard.h>
#include <time/Time.h>

#define READAHEAD_ENABLED 0

#ifdef SCSI_DEBUG
#define SCSI_DEBUG_LOG DEBUG_LOG
#else
#define SCSI_DEBUG_LOG(...)
#endif

void ScsiDisk::cacheCallback(CacheConstants::CallbackCause cause, uintptr_t loc, uintptr_t page, void *meta)
{
    ScsiDisk *pDisk = reinterpret_cast<ScsiDisk *>(meta);

    switch(cause)
    {
        case CacheConstants::WriteBack:
            {
                // Blocking write request.
                pDisk->flush(loc);
            }
            break;
        case CacheConstants::Eviction:
            // no-op for ScsiDisk
            break;
        default:
            WARNING("ScsiDisk: unknown cache callback -- could indicate potential future I/O issues.");
            break;
    }
}

ScsiDisk::ScsiDisk() :
    Disk(), m_Cache(), m_Inquiry(0), m_nAlignPoints(0), m_NumBlocks(0), m_BlockSize(0x1000), m_DeviceType(NoDevice)
{
    m_Cache.setCallback(cacheCallback, this);
}

ScsiDisk::~ScsiDisk()
{
}

bool ScsiDisk::initialise(ScsiController *pController, size_t nUnit)
{
    m_pController = pController;
    m_pParent = pController;
    m_nUnit = nUnit;

    m_Inquiry = new Inquiry;

    // Inquire as to the device's state
    /// \todo Use this data to change how read() and write() work
    ScsiCommand *pCommand = new ScsiCommands::Inquiry(sizeof(Inquiry), false);
    bool success = sendCommand(pCommand, reinterpret_cast<uintptr_t>(m_Inquiry), sizeof(Inquiry));
    if(!success)
    {
        ERROR("ScsiDisk: INQUIRY failed!");
        delete pCommand;
        return false;
    }
    delete pCommand;

    // Get the peripheral type out of the data.
    m_DeviceType = static_cast<ScsiPeripheralType>(m_Inquiry->Peripheral & 0x1F);

    // Ensure the unit is ready before we attempt to do anything more
    if(!unitReady())
    {
        // Grab sense data
        Sense *s = new Sense;
        PointerGuard<Sense> guard2(s);
        readSense(s);
        SCSI_DEBUG_LOG("ScsiDisk: Unit not yet ready, sense data: [sk=" << s->SenseKey << ", asc=" << s->Asc << ", ascq=" << s->AscQ << "]");

        if(s->SenseKey == 0x2)
        {
            if(s->Asc == 0x4)
            {
                if(s->AscQ == 0x2) // Logical Unit Not Ready, START UNIT Required
                {
                    // Start the unit
                    pCommand = new ScsiCommands::StartStop(false, true, 1, true);
                    success = sendCommand(pCommand, 0, 0, true);
                    if(!success)
                    {
                        readSense(s);
                        ERROR("ScsiDisk: unit startup failed! Sense data: [sk=" << s->SenseKey << ", asc=" << s->Asc << ", ascq=" << s->AscQ << "]");
                    }
                    delete pCommand;
                }
            }
        }

        Time::delay(100 * Time::Multiplier::MILLISECOND);

        // Attempt to see if the unit is ready again
        if(!unitReady())
        {
            readSense(s);
            SCSI_DEBUG_LOG("ScsiDisk: Unit not yet ready, sense data: [sk=" << s->SenseKey << ", asc=" << s->Asc << ", ascq=" << s->AscQ << "]");

            Time::delay(100 * Time::Multiplier::MILLISECOND);

            if(!unitReady())
            {
                readSense(s);
                ERROR("ScsiDisk: disk never became ready. Sense data: [sk=" << s->SenseKey << ", asc=" << s->Asc << ", ascq=" << s->AscQ << "]");
                return false;
            }
        }
    }

    // Get the capacity of the device
    getCapacityInternal(&m_NumBlocks, &m_NativeBlockSize);
    SCSI_DEBUG_LOG("ScsiDisk: Capacity: " << Dec << m_NumBlocks << " blocks, each " << m_NativeBlockSize << " bytes - " << (m_NativeBlockSize * m_NumBlocks) << Hex << " bytes in total.");

    // Chat to the partition service and let it pick up that we're around now
    ServiceFeatures *pFeatures = ServiceManager::instance().enumerateOperations(String("partition"));
    Service         *pService  = ServiceManager::instance().getService(String("partition"));
    if(pFeatures && pFeatures->provides(ServiceFeatures::touch))
    {
        NOTICE("Attempting to inform the partitioner of our presence...");
        if(pService)
        {
            if(pService->serve(ServiceFeatures::touch, static_cast<Disk*>(this), sizeof(static_cast<Disk*>(this))))
                NOTICE("Successful.");
            else
                ERROR("Failed.");
        }
        else
            ERROR("ScsiDisk: Couldn't tell the partition service about the new disk presence");
    }
    else
        ERROR("ScsiDisk: Partition service doesn't appear to support touch");
    return true;
}

bool ScsiDisk::readSense(Sense *sense)
{
    ByteSet(sense, 0xFF, sizeof(Sense));

    // Maximum size of sense data is 252 bytes
    ScsiCommand *pCommand = new ScsiCommands::ReadSense(0, sizeof(Sense));

    uint8_t *response = new uint8_t[sizeof(Sense)];
    bool success = sendCommand(pCommand, reinterpret_cast<uintptr_t>(response), sizeof(Sense));
    if(!success)
    {
        WARNING("ScsiDisk: SENSE command failed");
        delete [] response;
        return false;
    }

    /// \todo get the amount of data received from the SCSI device
    MemoryCopy(sense, response, sizeof(Sense));

    delete [] response;

    return ((sense->ResponseCode & 0x70) == 0x70);
}

bool ScsiDisk::unitReady()
{
    ScsiCommand *pCommand = new ScsiCommands::UnitReady();
    bool success = sendCommand(pCommand, 0, 0);
    delete pCommand;

    /// \todo this can fail with UNIT_ATTN or NOT_READY if the device is removable.
    return success;
}

bool ScsiDisk::getCapacityInternal(size_t *blockNumber, size_t *blockSize)
{
    if(!unitReady())
    {
        WARNING("ScsiDisk::getCapacityInternal - returning to defaults, unit not ready");
        *blockNumber = 0;
        *blockSize = defaultBlockSize();
        return false;
    }
    
    Capacity *capacity = new Capacity;
    PointerGuard<Capacity> guard(capacity);
    ByteSet(capacity, 0, sizeof(Capacity));

    ScsiCommand *pCommand = new ScsiCommands::ReadCapacity10();
    bool success = sendCommand(pCommand, reinterpret_cast<uintptr_t>(capacity), sizeof(Capacity), false);
    delete pCommand;
    if(!success)
    {
        WARNING("ScsiDisk::getCapacityInternal - READ CAPACITY command failed");
        return false;
    }

    *blockNumber = BIG_TO_HOST32(capacity->LBA);
    uint32_t blockSz = BIG_TO_HOST32(capacity->BlockSize);
    *blockSize = blockSz ? blockSz : defaultBlockSize();

    return true;
}

bool ScsiDisk::sendCommand(ScsiCommand *pCommand, uintptr_t pRespBuffer, uint16_t nRespBytes, bool bWrite)
{
    uintptr_t pCommandBuffer = 0;
    size_t nCommandSize = pCommand->serialise(pCommandBuffer);
    return m_pController->sendCommand(m_nUnit, pCommandBuffer, nCommandSize, pRespBuffer, nRespBytes, bWrite);
}

uintptr_t ScsiDisk::read(uint64_t location)
{
    if (!(getBlockSize() && getNativeBlockSize()))
    {
        ERROR("ScsiDisk::read - block size is zero.");
        return 0;
    }

    size_t endLocation = location + getNativeBlockSize();
    if(endLocation > getSize())
    {
        ERROR("ScsiDisk::read - location too high (end location " << endLocation << " > " << getSize() << ")");
        return 0;
    }

    size_t blockNum = location / getNativeBlockSize();
    if(blockNum > getBlockCount())
    {
        ERROR("ScsiDisk::read - location too high (block " << blockNum << " > " << getBlockCount() << ")");
        return 0;
    }

    // Look through the align points
    uint64_t alignPoint = 0;
    for (size_t i = 0; i < m_nAlignPoints; i++)
        if (m_AlignPoints[i] <= location && m_AlignPoints[i] > alignPoint)
            alignPoint = m_AlignPoints[i];

    // Calculate the offset to get location on a page boundary.
    ssize_t offs =  -((location - alignPoint) % 4096);

    uintptr_t buffer;
    if((buffer = m_Cache.lookup(location + offs)))
    {
        return buffer - offs;
    }

    // Grab an aligned location to read.
    size_t loc = (location + offs) & ~(getBlockSize() - 1);

    ScsiController *pParent = static_cast<ScsiController*> (m_pParent);
    pParent->addRequest(0, SCSI_REQUEST_READ, reinterpret_cast<uint64_t> (this), loc);
#if READAHEAD_ENABLED
    // Queue up a few readaheads, assuming sequential access.
    for (size_t i = 0; i < (0x20000 / getBlockSize()); ++i)
    {
        pParent->addAsyncRequest(0, SCSI_REQUEST_READ, reinterpret_cast<uint64_t> (this), loc + (getBlockSize() * i));
    }
#endif
    return m_Cache.lookup(location + offs) - offs;
}

void ScsiDisk::write(uint64_t location)
{
#ifndef CRIPPLE_HDD
    if (!(getBlockSize() && getNativeBlockSize()))
    {
        ERROR("ScsiDisk::write - block size is zero.");
        return;
    }

    if((location + getNativeBlockSize()) > getSize())
    {
        ERROR("ScsiDisk::write - location too high");
        ERROR(" -> " << location << " vs " << getSize());
        return;
    }

    if((location / getNativeBlockSize()) > getBlockCount())
    {
        ERROR("ScsiDisk::write - location too high");
        ERROR(" -> block " << (location / getNativeBlockSize()) << " vs " << getBlockCount());
        return;
    }

    // Look through the align points
    uint64_t alignPoint = 0;
    for (size_t i = 0; i < m_nAlignPoints; i++)
        if (m_AlignPoints[i] <= location && m_AlignPoints[i] > alignPoint)
            alignPoint = m_AlignPoints[i];

    // Calculate the offset to get location on a page boundary.
    ssize_t offs =  -((location - alignPoint) % 4096);

    uintptr_t buffer;
    if(!(buffer = m_Cache.lookup(location + offs)))
    {
        ERROR("ScsiDisk::write - no buffer!");
        return;
    }

    // Implicit pin caused by lookup() must be released by the write request
    // handler, to ensure the refcount is correct.
    ScsiController *pParent = static_cast<ScsiController*> (m_pParent);
    pParent->addAsyncRequest(0, SCSI_REQUEST_WRITE, reinterpret_cast<uint64_t> (this), location + offs);
#endif
}

void ScsiDisk::flush(uint64_t location)
{
#ifndef CRIPPLE_HDD
    if (!(getBlockSize() && getNativeBlockSize()))
    {
        ERROR("ScsiDisk::flush - block size is zero.");
        return;
    }

    if((location + getNativeBlockSize()) > getSize())
    {
        ERROR("ScsiDisk::flush - location too high");
        return;
    }

    if((location / getNativeBlockSize()) > getBlockCount())
    {
        ERROR("ScsiDisk::flush - location too high");
        return;
    }

    // Look through the align points
    uint64_t alignPoint = 0;
    for (size_t i = 0; i < m_nAlignPoints; i++)
        if (m_AlignPoints[i] <= location && m_AlignPoints[i] > alignPoint)
            alignPoint = m_AlignPoints[i];

    // Calculate the offset to get location on a page boundary.
    ssize_t offs =  -((location - alignPoint) % 4096);

    uintptr_t buffer;
    if(!(buffer = m_Cache.lookup(location + offs)))
    {
        return;
    }

    // Make sure this page remains for both the write AND the sync.
    m_Cache.pin(location + offs);

    ScsiController *pParent = static_cast<ScsiController*> (m_pParent);
    pParent->addRequest(0, SCSI_REQUEST_WRITE, reinterpret_cast<uint64_t> (this), location + offs);
    pParent->addRequest(0, SCSI_REQUEST_SYNC, reinterpret_cast<uint64_t> (this), location + offs);

    // Undo our pin for write+sync.
    m_Cache.release(location + offs);
#endif
}

void ScsiDisk::align(uint64_t location)
{
    assert(m_nAlignPoints < 8);
    m_AlignPoints[m_nAlignPoints++] = location;
}

uint64_t ScsiDisk::doRead(uint64_t location)
{
    // Wait for the unit to be ready before reading
    bool bReady = false;
    for(int i = 0; i < 3; i++)
    {
        if((bReady = unitReady()))
            break;
    }

    if(!bReady)
    {
        ERROR("ScsiDisk::doRead - unit not ready");
        return 0;
    }

    // Handle the case where a read took place while we were waiting in the
    // RequestQueue - don't double up the cache.
    uintptr_t buffer = m_Cache.lookup(location);
    if(buffer)
    {
        WARNING("ScsiDisk::doRead(" << location << ") - buffer was already in cache");
        m_Cache.release(location);
        return 0;
    }
    buffer = m_Cache.insert(location, getBlockSize());
    if(!buffer)
    {
        FATAL("ScsiDisk::doRead - no buffer");
    }

    size_t blockNum = location / getNativeBlockSize();
    size_t blockCount = getBlockSize() / getNativeBlockSize();

    bool bOk;
    ScsiCommand *pCommand;

    // TOC?
    if (m_DeviceType == CdDvdDevice)
    {
        /// \todo Cache this somewhere.
        pCommand = new ScsiCommands::ReadTocCommand(getNativeBlockSize());
        uint8_t *toc = new uint8_t[getNativeBlockSize()];
        PointerGuard<uint8_t> tmpBuffGuard(toc, true);
        bOk = sendCommand(pCommand, reinterpret_cast<uintptr_t>(toc), getNativeBlockSize());
        delete pCommand;
        if (!bOk)
        {
            WARNING("ScsiDisk::doRead - could not find data track (READ TOC failed)");
            return 0;
        }

        uint16_t i;
        bool bHaveTrack = false;
        /// \todo endianness issue?
        uint16_t bufLen = BIG_TO_HOST16((toc[0] << 8) | toc[1]);
        ScsiCommands::ReadTocCommand::TocEntry *Toc = reinterpret_cast<ScsiCommands::ReadTocCommand::TocEntry*>(toc + 4);
        for(i = 0; i < (bufLen / 8); i++)
        {
            if(Toc[i].Flags == 0x14)
            {
                bHaveTrack = true;
                break;
            }
        }

        if(!bHaveTrack)
        {
            WARNING("ScsiDisk::doRead - could not find data track (no data track)");
            return 0;
        }

        uint32_t trackStart = BIG_TO_HOST32(Toc[i].TrackStart);
        if((blockNum + trackStart) < blockNum)
        {
            WARNING("ScsiDisk::doRead - TOC overflow");
            return 0;
        }

        blockNum += trackStart;
    }

    for(int i = 0; i < 3; i++)
    {
        SCSI_DEBUG_LOG("SCSI: trying read(10)");
        pCommand = new ScsiCommands::Read10(blockNum, blockCount);
        bOk = sendCommand(pCommand, buffer, getBlockSize());
        delete pCommand;
        if(bOk)
            return 0;
    }
    for(int i = 0; i < 3; i++)
    {
        SCSI_DEBUG_LOG("SCSI: trying read(12)");
        pCommand = new ScsiCommands::Read12(blockNum, blockCount);
        bOk = sendCommand(pCommand, buffer, getBlockSize());
        delete pCommand;
        if(bOk)
            return 0;
    }
    for(int i = 0; i < 3; i++)
    {
        SCSI_DEBUG_LOG("SCSI: trying read(16)");
        pCommand = new ScsiCommands::Read16(blockNum, blockCount);
        bOk = sendCommand(pCommand, buffer, getBlockSize());
        delete pCommand;
        if(bOk)
            return 0;
    }

    ERROR("SCSI: reading failed?");

    return 0;
}

uint64_t ScsiDisk::doWrite(uint64_t location)
{
    // Wait for the unit to be ready before writing
    bool bReady = false;
    for(int i = 0; i < 3; i++)
    {
        if((bReady = unitReady()))
            break;
    }

    if(!bReady)
    {
        ERROR("ScsiDisk::doWrite - unit not ready");
        return 0;
    }

    // Handle the case where a read took place while we were waiting in the
    // RequestQueue - don't double up the cache.
    uintptr_t buffer = m_Cache.lookup(location);
    if(!buffer)
    {
        WARNING("ScsiDisk::doWrite(" << location << ") - buffer was not in cache");
        return 0;
    }

    // Make sure we don't hold the refcnt once we exit this method.
    CachePageGuard guard(m_Cache, location);

    size_t block = location / getNativeBlockSize();
    size_t count = 4096 / getNativeBlockSize();

    bool bOk;
    ScsiCommand *pCommand;

    for(int i = 0; i < 3; i++)
    {
        SCSI_DEBUG_LOG("SCSI: trying write(10)");
        pCommand = new ScsiCommands::Write10(block, count);
        bOk = sendCommand(pCommand, buffer, 4096, true);
        delete pCommand;
        if(bOk)
            break;
    }
    if(!bOk)
    {
        for(int i = 0; i < 3; i++)
        {
            SCSI_DEBUG_LOG("SCSI: trying write(12)");
            pCommand = new ScsiCommands::Write12(block, count);
            bOk = sendCommand(pCommand, buffer, 4096, true);
            delete pCommand;
            if(bOk)
                break;
        }
    }
    if(!bOk)
    {
        for(int i = 0; i < 3; i++)
        {
            SCSI_DEBUG_LOG("SCSI: trying write(16)");
            pCommand = new ScsiCommands::Write16(block, count);
            bOk = sendCommand(pCommand, buffer, 4096, true);
            delete pCommand;
            if(bOk)
                break;
        }
    }

    if(!bOk)
    {
        ERROR("SCSI: writing failed?");
    }

    return 0;
}

uint64_t ScsiDisk::doSync(uint64_t location)
{
    // Wait for the unit to be ready before writing
    bool bReady = false;
    for(int i = 0; i < 3; i++)
    {
        if((bReady = unitReady()))
            break;
    }

    if(!bReady)
    {
        ERROR("ScsiDisk::doSync - unit not ready");
        return 0;
    }

    size_t block = location / getNativeBlockSize();
    size_t count = 4096 / getNativeBlockSize();

    bool bOk;
    ScsiCommand *pCommand;

    // Kick off a synchronise (this will be slow, but will ensure the data is on disk)
    for(int i = 0; i < 3; i++)
    {
        SCSI_DEBUG_LOG("SCSI: trying synchronise(10)");
        pCommand = new ScsiCommands::Synchronise10(block, count);
        bOk = sendCommand(pCommand, 0, 0);
        delete pCommand;
        if(bOk)
            break;
    }

    if(!bOk)
    {
        for(int i = 0; i < 3; i++)
        {
            SCSI_DEBUG_LOG("SCSI: trying synchronise(16)");
            pCommand = new ScsiCommands::Synchronise16(block, count);
            bOk = sendCommand(pCommand, 0, 0);
            delete pCommand;
            if(bOk)
                break;
        }
    }

    return 0;
}

void ScsiDisk::pin(uint64_t location)
{
    m_Cache.pin(location);
}

void ScsiDisk::unpin(uint64_t location)
{
    m_Cache.release(location);
}

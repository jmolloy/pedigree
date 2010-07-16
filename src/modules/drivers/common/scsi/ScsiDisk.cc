/*
 * Copyright (c) 2010 Eduard Burtescu
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

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

ScsiDisk::ScsiDisk() : m_Cache(), m_nAlignPoints(0), m_NumBlocks(0), m_BlockSize(0)
{
}

ScsiDisk::~ScsiDisk()
{
}

bool ScsiDisk::initialise(ScsiController *pController, size_t nUnit)
{
    m_pController = pController;
    m_pParent = pController;
    m_nUnit = nUnit;

    // Turn on the unit - go ACTIVE
    ScsiCommand *pCommand = new ScsiCommands::StartStop(false, 1, true, true);
    bool success = sendCommand(pCommand, 0, 0);
    if(!success)
        return false;

    // Ensure the unit is ready before we attempt to do anything more
    if(!unitReady())
    {
        ERROR("ScsiDisk: disk never became ready.");
        return false;
    }

    // Get the capacity of the device
    getCapacityInternal(&m_NumBlocks, &m_BlockSize);
    DEBUG_LOG("ScsiDisk: Capacity: " << Dec << m_NumBlocks << " blocks, each " << m_BlockSize << " bytes - " << (m_BlockSize * m_NumBlocks) << Hex << " bytes in total.");

    // Chat to the partition service and let it pick up that we're around now
    ServiceFeatures *pFeatures = ServiceManager::instance().enumerateOperations(String("partition"));
    Service         *pService  = ServiceManager::instance().getService(String("partition"));
    NOTICE("Asking if the partition provider supports touch");
    if(pFeatures->provides(ServiceFeatures::touch))
    {
        NOTICE("It does, attempting to inform the partitioner of our presence...");
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
    memset(sense, 0xFF, sizeof(Sense));

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
    memcpy(sense, response, sizeof(Sense));

    // Dump the sense information
    DEBUG_LOG("    Sense information:");
    for(size_t i = 0; i < sizeof(Sense); i++)
        DEBUG_LOG("        [" << Dec << i << Hex << "] " << response[i]);

    delete [] response;

    return ((sense->ResponseCode & 0x70) == 0x70);
}

bool ScsiDisk::unitReady()
{
    Sense s;
    memset(&s, 0, sizeof(Sense));

    ScsiCommand *pCommand = new ScsiCommands::UnitReady();

    readSense(&s);

    bool success = false;
    int retry = 5;
    do
    {
        success = sendCommand(pCommand, 0, 0, true);

        if(!success)
            delay(100);
    } while((!success) && (readSense(&s)) && --retry);

    delete pCommand;

    return (((s.ResponseCode & 0x70) == 0x70) && ((s.SenseKey == 0x06) || (s.SenseKey == 0x02) || (s.SenseKey == 0x00)));
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

    struct Capacity
    {
        uint32_t LBA;
        uint32_t BlockSize;
    } __attribute__((packed)) capacity;
    memset(&capacity, 0, sizeof(Capacity));

    ScsiCommand *pCommand = new ScsiCommands::ReadCapacity10();
    bool success = sendCommand(pCommand, reinterpret_cast<uintptr_t>(&capacity), sizeof(Capacity), false);
    delete pCommand;
    if(!success)
    {
        WARNING("ScsiDisk::getCapacityInternal - READ CAPACITY command failed");
        return false;
    }

    *blockNumber = BIG_TO_HOST32(capacity.LBA);
    uint32_t blockSz = BIG_TO_HOST32(capacity.BlockSize);
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
    if (location % 512)
        FATAL("Read with location % 512.");
    if((location / m_BlockSize) > m_NumBlocks)
    {
        ERROR("ScsiDisk::read - location too high");
        return 0;
    }

    // Look through the align points.
    uint64_t alignPoint = 0;
    for (size_t i = 0; i < m_nAlignPoints; i++)
        if (m_AlignPoints[i] <= location && m_AlignPoints[i] > alignPoint)
            alignPoint = m_AlignPoints[i];
    alignPoint %= 4096;

    // Determine which page the read is in
    uint64_t pageNumber = ((location - alignPoint) & ~0xFFFUL) + alignPoint;
    uint64_t pageOffset = (location - alignPoint) % 4096;

    uintptr_t buffer = m_Cache.lookup(pageNumber);

    if (buffer)
        return buffer + pageOffset;

    buffer = m_Cache.insert(pageNumber);

    bool bOk;
    ScsiCommand *pCommand;

	// Wait for the unit to be ready before reading
    if(!unitReady())
    {
        ERROR("ScsiDisk::read - unit not ready");
        return 0;
    }

    DEBUG_LOG("SCSI: trying read(10)");
    pCommand = new ScsiCommands::Read10(pageNumber / m_BlockSize, 4096 / m_BlockSize);
    bOk = sendCommand(pCommand, buffer, 4096);
    delete pCommand;
    if(bOk)
        return buffer + pageOffset;
    DEBUG_LOG("SCSI: trying read(12)");
    pCommand = new ScsiCommands::Read12(pageNumber / m_BlockSize, 4096 / m_BlockSize);
    bOk = sendCommand(pCommand, buffer, 4096);
    delete pCommand;
    if(bOk)
        return buffer + pageOffset;
    DEBUG_LOG("SCSI: trying read(16)");
    pCommand = new ScsiCommands::Read16(pageNumber / m_BlockSize, 4096 / m_BlockSize);
    bOk = sendCommand(pCommand, buffer, 4096);
    delete pCommand;
    if(bOk)
        ERROR("SCSI: no read function worked");

    return buffer + pageOffset;
}

void ScsiDisk::write(uint64_t location)
{
    WARNING("ScsiDisk::write: Not implemented.");
}

void ScsiDisk::align(uint64_t location)
{
    assert(m_nAlignPoints < 8);
    m_AlignPoints[m_nAlignPoints++] = location;
}

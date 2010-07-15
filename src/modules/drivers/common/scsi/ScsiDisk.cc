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

#include <utilities/assert.h>
#include <utilities/Cache.h>
#include <ServiceManager.h>
#include "ScsiDisk.h"
#include "ScsiCommands.h"
#include "ScsiController.h"

ScsiDisk::ScsiDisk() : m_Cache(), m_nAlignPoints(0)
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

bool ScsiDisk::sendCommand(ScsiCommand *pCommand, uintptr_t pRespBuffer, uint16_t nRespBytes, bool bWrite)
{
    uintptr_t pCommandBuffer = 0;
    size_t nCommandSize = pCommand->serialise(pCommandBuffer);
    return m_pController->sendCommand(m_nUnit, pCommandBuffer, nCommandSize, pRespBuffer, nRespBytes, bWrite);
}
/*
void ScsiDisk::readPage(uint64_t sector, uintptr_t buffer) {
    Cbw *cb = new Cbw();
    cb->sig = 0x43425355;
    cb->tag = 0;
    cb->data_len = 4096;
    cb->flags = 0x80;
    cb->lun = 0;
    cb->cmd_len = 6;
    cb->cmd[0] = 8;
    cb->cmd[1] = (sector >> 16) & 0x1f;
    cb->cmd[2] = (sector >> 8) & 0xff;
    cb->cmd[3] = sector & 0xff;
    cb->cmd[4] = 8;
    cb->cmd[5] = 0;

    syncOut(2, cb, 31);
    for(size_t i = 0;i<4096;i+=64){
        //NOTICE("start read "<<(i/64));
        ssize_t lol = syncIn(1, reinterpret_cast<void*>(buffer+i), 64);
        if(lol < 0)
            ERROR("done read "<<(i/64)<<" as "<<lol);
    }
    if(syncIn(1, 0, 13) < 0)
        readPage(sector, buffer);
}*/

uintptr_t ScsiDisk::read(uint64_t location)
{
    if (location % 512)
        FATAL("Read with location % 512.");

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

    NOTICE("SCSI: trying read(10)");
    pCommand = new ScsiCommands::Read10(pageNumber / 512, 8);
    bOk = sendCommand(pCommand, buffer, 4096);
    delete pCommand;
    if(bOk)
        return buffer + pageOffset;
    NOTICE("SCSI: trying read(12)");
    pCommand = new ScsiCommands::Read12(pageNumber / 512, 8);
    bOk = sendCommand(pCommand, buffer, 4096);
    delete pCommand;
    if(bOk)
        return buffer + pageOffset;
    NOTICE("SCSI: trying read(16)");
    pCommand = new ScsiCommands::Read16(pageNumber / 512, 8);
    bOk = sendCommand(pCommand, buffer, 4096);
    delete pCommand;
    if(bOk)
        ERROR("SCSI: none worked");

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

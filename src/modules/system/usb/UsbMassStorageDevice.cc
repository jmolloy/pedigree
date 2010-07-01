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

#include <machine/Disk.h>
#include <usb/UsbDevice.h>
#include <usb/UsbMassStorageDevice.h>
#include <utilities/assert.h>
#include <utilities/Cache.h>
#include <ServiceManager.h>

UsbMassStorageDevice::UsbMassStorageDevice(UsbDevice *dev) : Device(dev), UsbDevice(dev), m_nUnits(1)
{
    searchDisks();
}

UsbMassStorageDevice::~UsbMassStorageDevice()
{
}
#define nPacketSize 64
void UsbMassStorageDevice::sendCommand(size_t nUnit, uintptr_t pCommand, uint8_t nCommandSize, uintptr_t pRespBuffer, uint16_t nRespBytes, bool bWrite)
{
    Cbw *cb = new Cbw();
    cb->sig = 0x43425355;
    cb->tag = 0;
    cb->data_len = nRespBytes;
    cb->flags = bWrite?0:0x80;
    cb->lun = nUnit;
    cb->cmd_len = nCommandSize;
    memcpy(cb->cmd, reinterpret_cast<void*>(pCommand), nCommandSize);

    ssize_t nResult = syncOut(2, reinterpret_cast<uintptr_t>(cb), 31);
    NOTICE("STAGE1 "<<Dec<<nResult<<Hex);

    if(bWrite)
        nResult = syncOut(2, pRespBuffer, nRespBytes);
    else
        nResult = syncIn(1, pRespBuffer, nRespBytes);
    NOTICE("STAGE2 "<<Dec<<nResult<<Hex);

    Csw myCs;
    nResult = syncIn(1, reinterpret_cast<uintptr_t>(&myCs), 13);
    NOTICE("STAGE3 "<<Dec<<nResult<<Hex);

    NOTICE("FUN "<<((uint8_t*)pRespBuffer)[510]<<" "<<((uint8_t*)pRespBuffer)[511]<<" "<<myCs.status);
}
/*
void UsbMassStorageDevice::readPage(uint64_t sector, uintptr_t buffer) {
    Cbw *cb = new Cbw();
    cb->sig = 0x43425355;
    cb->tag = 0;
    cb->data_len = 4096;
    cb->flags = 0x80;
    cb->lun = 0;
    cb->cmd_len = 10;
    cb->cmd[0] = 0x28;
    cb->cmd[1] = 0;
    cb->cmd[2] = (sector >> 24) & 0xff;
    cb->cmd[3] = (sector >> 16) & 0xff;
    cb->cmd[4] = (sector >> 8) & 0xff;
    cb->cmd[5] = sector & 0xff;
    cb->cmd[6] = 0;
    cb->cmd[7] = 0;
    cb->cmd[8] = 8;
    cb->cmd[9] = 0;

    int lol = syncOut(2, reinterpret_cast<uintptr_t>(cb), 31);
    bool success = true;
    for(size_t i = 0;i<4096;i+=512){
        ssize_t lol = syncIn(1, buffer+i, 512);
        if(lol < 0) {
            ERROR("ERROR: done read "<<Dec<<(i/512)<<" as "<<lol<<Hex);
            success = false;
            break;
        }
    }
    Csw myCs;
    if(syncIn(1, reinterpret_cast<uintptr_t>(&myCs), 13) < 0){
        ERROR("ERROR: oops");
        readPage(sector, buffer);
        }
        NOTICE("heh "<<((uint8_t*)buffer)[510]<<" "<<((uint8_t*)buffer)[511]<<" "<<myCs.status);
}
*/

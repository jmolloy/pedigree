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

UsbMassStorageDevice::UsbMassStorageDevice(UsbDevice *dev) : Device(dev), UsbDevice(dev), m_nUnits(1), m_nInEndpoint(0), m_nOutEndpoint(0)
{
    for(uint8_t i = 1;i<16;i++)
    {
        Endpoint *pEndpoint = m_pEndpoints[i];
        if(!m_nInEndpoint && pEndpoint->nTransferType == Endpoint::Bulk && pEndpoint->bIn)
            m_nInEndpoint = i;
        if(!m_nOutEndpoint && pEndpoint->nTransferType == Endpoint::Bulk && pEndpoint->bOut)
            m_nOutEndpoint = i;
        if(m_nInEndpoint && m_nOutEndpoint)
            break;
    }

    if(!m_nInEndpoint)
    {
        ERROR("USB: MSD: No IN endpoint");
        return;
    }

    if(!m_nOutEndpoint)
    {
        ERROR("USB: MSD: No OUT endpoint");
        return;
    }

    searchDisks();
}

UsbMassStorageDevice::~UsbMassStorageDevice()
{
}

bool UsbMassStorageDevice::sendCommand(size_t nUnit, uintptr_t pCommand, uint8_t nCommandSize, uintptr_t pRespBuffer, uint16_t nRespBytes, bool bWrite)
{
    Cbw *pCbw = new Cbw();
    memset(pCbw, 0, sizeof(Cbw));
    pCbw->sig = HOST_TO_LITTLE32(0x43425355);
    pCbw->tag = 0;
    pCbw->data_len = HOST_TO_LITTLE32(nRespBytes);
    pCbw->flags = bWrite ? 0 : 0x80;
    pCbw->lun = nUnit;
    pCbw->cmd_len = nCommandSize;
    memcpy(pCbw->cmd, reinterpret_cast<void*>(pCommand), nCommandSize);

    ssize_t nResult = syncOut(m_nOutEndpoint, reinterpret_cast<uintptr_t>(pCbw), 31);

    NOTICE("USB: MSD: CBW finished with " << Dec << nResult << Hex);

    if(nResult < 0)
        return false;

    if(nRespBytes)
    {
        if(bWrite)
            nResult = syncOut(m_nOutEndpoint, pRespBuffer, nRespBytes);
        else
            nResult = syncIn(m_nInEndpoint, pRespBuffer, nRespBytes);

        NOTICE("USB: MSD: "<< (bWrite ? "OUT" : "IN") << " finished with  " << Dec << nResult << Hex);

        if(nResult < 0)
            return false;
    }

    Csw csw;
    nResult = syncIn(m_nInEndpoint, reinterpret_cast<uintptr_t>(&csw), 13);
    NOTICE("USB: MSD: CSW finished with "<<Dec<<nResult<<Hex);

    NOTICE("USB: MSD: Command finished 510:511="<<((uint8_t*)pRespBuffer)[510]<<":"<<((uint8_t*)pRespBuffer)[511]<<" STS="<<csw.status<<" SIG="<<csw.sig);
    return !csw.status;
}

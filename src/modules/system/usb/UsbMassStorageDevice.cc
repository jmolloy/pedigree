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
#include <usb/UsbHub.h>
#include <usb/UsbMassStorageDevice.h>
#include <utilities/assert.h>
#include <utilities/Cache.h>
#include <ServiceManager.h>
#include <utilities/PointerGuard.h>

UsbMassStorageDevice::UsbMassStorageDevice(UsbDevice *dev) : Device(dev), UsbDevice(dev), m_nUnits(1), m_nInEndpoint(0), m_nOutEndpoint(0)
{
    for(uint8_t i = 0; i < m_pInterface->pEndpoints.count(); i++)
    {
        Endpoint *pEndpoint = m_pEndpoints[i + 1];
        if(!m_nInEndpoint && (pEndpoint->nTransferType == Endpoint::Bulk) && pEndpoint->bIn)
            m_nInEndpoint = i + 1;
        if(!m_nOutEndpoint && (pEndpoint->nTransferType == Endpoint::Bulk) && pEndpoint->bOut)
            m_nOutEndpoint = i + 1;
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

    // Reset the mass storage device and associated interface
    controlRequest(0x21, 0xFF, 0, m_pInterface->pDescriptor->nInterface, 0, 0);

    searchDisks();
}

UsbMassStorageDevice::~UsbMassStorageDevice()
{
}

bool UsbMassStorageDevice::massStorageReset()
{
    return !controlRequest(0x21, 0xFF, 0, m_pInterface->pDescriptor->nInterface, 0, 0);
}

bool UsbMassStorageDevice::clearEndpointHalt(uint8_t nEndpoint)
{
    return !controlRequest(0x2, 1, 0, nEndpoint, 0, 0);
}

bool UsbMassStorageDevice::sendCommand(size_t nUnit, uintptr_t pCommand, uint8_t nCommandSize, uintptr_t pRespBuffer, uint16_t nRespBytes, bool bWrite)
{
    Cbw *pCbw = new Cbw();
    PointerGuard<Cbw> guard(pCbw);
    memset(pCbw, 0, sizeof(Cbw));
    pCbw->sig = HOST_TO_LITTLE32(0x43425355);
    pCbw->tag = 0;
    pCbw->data_len = HOST_TO_LITTLE32(nRespBytes);
    pCbw->flags = bWrite ? 0 : 0x80;
    pCbw->lun = nUnit;
    pCbw->cmd_len = nCommandSize;
    memcpy(pCbw->cmd, reinterpret_cast<void*>(pCommand), nCommandSize);

    ssize_t nResult = syncOut(m_nOutEndpoint, reinterpret_cast<uintptr_t>(pCbw), 31);

    DEBUG_LOG("USB: MSD: CBW finished with " << Dec << nResult << Hex);

    // Handle stall
    if(nResult == -0x40)
    {
        // Clear out pipe
        if(!clearEndpointHalt(m_nOutEndpoint))
        {
            // Reset and fail this command
            massStorageReset();
            clearEndpointHalt(m_nInEndpoint);
            clearEndpointHalt(m_nOutEndpoint);
            return false;
        }
        else
            nResult = 0; // Attempt data transfer
    }

    if(nResult < 0)
        return false;

    // Handle data or CSW transfer if needed
    if(nRespBytes)
    {
        DEBUG_LOG("USB: MSD: Performing " << Dec << nRespBytes << Hex << " byte " << (bWrite ? "write" : "read"));
        if(bWrite)
            nResult = syncOut(m_nOutEndpoint, pRespBuffer, nRespBytes);
        else
            nResult = syncIn(m_nInEndpoint, pRespBuffer, nRespBytes);

        DEBUG_LOG("USB: MSD: Result: " << Dec << nResult << Hex << ".");

        if(nResult == -0x40) /// \todo Proper error return values
        {
            // STALL, clear the endpoint and attempt CSW read
            bool bClearResult = false;
            if(bWrite)
                bClearResult = !clearEndpointHalt(m_nOutEndpoint);
            else
                bClearResult = !clearEndpointHalt(m_nInEndpoint);

            if(!bClearResult)
            {
                DEBUG_LOG("USB: MSD: Endpoint stalled, but clearing failed. Performing reset.");

                // Reset and fail this command
                massStorageReset();
                clearEndpointHalt(m_nInEndpoint);
                clearEndpointHalt(m_nOutEndpoint);
                return false;
            }

            // Attempt to read the CSW now that the stall condition is cleared
            Csw csw;
            nResult = syncIn(m_nInEndpoint, reinterpret_cast<uintptr_t>(&csw), 13);

            // Stalled?
            if(nResult == -0x40)
            {
                // Perform full reset and reset both pipes
                massStorageReset();
                clearEndpointHalt(m_nInEndpoint);
                clearEndpointHalt(m_nOutEndpoint);

                // Failure condition
                DEBUG_LOG("USB: MSD: Couldn't recover cleanly from endpoint stall, mass storage reset completed");
                return false;
            }
            else if(nResult < 0)
            {
                DEBUG_LOG("USB: MSD: Reading CSW after clearing stall ended up failing with status " << nResult);
                return false;
            }
            else
            {
                DEBUG_LOG("USB: MSD: Recovered from endpoint stall");
                return !csw.status;
            }
        }

        if(nResult == 13)
        {
            Csw *pCsw = reinterpret_cast<Csw*>(pRespBuffer);
            if(pCsw->sig == HOST_TO_LITTLE32(0x53425355))
            {
                DEBUG_LOG("USB: MSD: Early CSW with status " << pCsw->status << ", residue: " << pCsw->residue);
                return !pCsw->status;
            }
        }

        if(nResult < 0)
            return false;
    }

    Csw *csw = new Csw;
    PointerGuard<Csw> guard2(csw);
    nResult = syncIn(m_nInEndpoint, reinterpret_cast<uintptr_t>(csw), 13);

    /// \todo Handle stall here
    if(nResult < 0)
    {
        DEBUG_LOG("USB: MSD: Reading CSW ended up failing with status " << nResult);
        return false;
    }

    DEBUG_LOG("USB: MSD: Command finished STS = " << csw->status << " SIG=" << csw->sig);

    return !csw->status;
}


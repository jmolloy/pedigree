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
#include <utilities/assert.h>
#include <utilities/Cache.h>
#include <utilities/PointerGuard.h>
#include <ServiceManager.h>
#include <usb/UsbDevice.h>
#include <usb/UsbHub.h>
#include "UsbMassStorageDevice.h"

UsbMassStorageDevice::UsbMassStorageDevice(UsbDevice *dev) :
    UsbDevice(dev), ScsiController(), m_nUnits(0), m_pInEndpoint(0), m_pOutEndpoint(0)
{
}

UsbMassStorageDevice::~UsbMassStorageDevice()
{
}

void UsbMassStorageDevice::initialiseDriver()
{
    for(size_t i = 0; i < m_pInterface->endpointList.count(); i++)
    {
        Endpoint *pEndpoint = m_pInterface->endpointList[i];
        if(!m_pInEndpoint && (pEndpoint->nTransferType == Endpoint::Bulk) && pEndpoint->bIn)
            m_pInEndpoint = pEndpoint;
        if(!m_pOutEndpoint && (pEndpoint->nTransferType == Endpoint::Bulk) && pEndpoint->bOut)
            m_pOutEndpoint = pEndpoint;
        if(m_pInEndpoint && m_pOutEndpoint)
            break;
    }

    if(!m_pInEndpoint)
    {
        ERROR("USB: MSD: No IN endpoint");
        return;
    }

    if(!m_pOutEndpoint)
    {
        ERROR("USB: MSD: No OUT endpoint");
        return;
    }

    // Reset the mass storage device and associated interface
    massStorageReset();

    // Get the maximum LUN and find out the number of units
    /// \todo Some mass storage devices don't support this command, fail to
    ///       return logical information, or just report incorrect data.
    ///       All that needs to be handled.
    uint8_t *nMaxLUN = new uint8_t(0);
    if(!controlRequest(UsbRequestDirection::In | MassStorageRequest, MassStorageGetMaxLUN, 0, m_pInterface->nInterface, 1, reinterpret_cast<uintptr_t>(nMaxLUN)))
    {
        ERROR("USB: MSD: Couldn't get maximum LUN");
        return;
    }
    m_nUnits = *nMaxLUN + 1;
    delete nMaxLUN;

    searchDisks();

    m_UsbState = HasDriver;
}

bool UsbMassStorageDevice::massStorageReset()
{
    return controlRequest(MassStorageRequest, MassStorageReset, 0, m_pInterface->nInterface);
}

bool UsbMassStorageDevice::sendCommand(size_t nUnit, uintptr_t pCommand, uint8_t nCommandSize, uintptr_t pRespBuffer, uint16_t nRespBytes, bool bWrite)
{
    Cbw *pCbw = new Cbw;
    PointerGuard<Cbw> guard(pCbw);
    memset(pCbw, 0, sizeof(Cbw));
    pCbw->nSig = CbwSig;
    pCbw->nDataBytes = HOST_TO_LITTLE32(nRespBytes);
    pCbw->nFlags = bWrite ? 0 : 0x80;
    pCbw->nLUN = nUnit;
    pCbw->nCommandSize = nCommandSize;
    memcpy(pCbw->pCommand, reinterpret_cast<void*>(pCommand), nCommandSize);

    ssize_t nResult = syncOut(m_pOutEndpoint, reinterpret_cast<uintptr_t>(pCbw), 31);

    DEBUG_LOG("USB: MSD: CBW finished with " << Dec << nResult << Hex);

    // Handle stall
    if(nResult == -Stall)
    {
        // Clear out pipe
        if(!clearEndpointHalt(m_pOutEndpoint))
        {
            // Reset and fail this command
            massStorageReset();
            clearEndpointHalt(m_pInEndpoint);
            clearEndpointHalt(m_pOutEndpoint);
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
            nResult = syncOut(m_pOutEndpoint, pRespBuffer, nRespBytes);
        else
            nResult = syncIn(m_pInEndpoint, pRespBuffer, nRespBytes);

        DEBUG_LOG("USB: MSD: Result: " << Dec << nResult << Hex << ".");

        /// \todo Should probably just be transaction errors and stalls
        if((nResult < 0) || ((nResult < nRespBytes) && (!bWrite))) // == -Stall)
        {
            // STALL, clear the endpoint and attempt CSW read
            bool bClearResult = false;
            if(bWrite)
                bClearResult = !clearEndpointHalt(m_pOutEndpoint);
            else
                bClearResult = !clearEndpointHalt(m_pInEndpoint);

            if(!bClearResult)
            {
                DEBUG_LOG("USB: MSD: Endpoint stalled, but clearing failed. Performing reset.");

                // Reset and fail this command
                massStorageReset();
                clearEndpointHalt(m_pInEndpoint);
                clearEndpointHalt(m_pOutEndpoint);
                return false;
            }

            // Attempt to read the CSW now that the stall condition is cleared
            Csw *pCsw = new Csw;
            PointerGuard<Csw> guard(pCsw);
            nResult = syncIn(m_pInEndpoint, reinterpret_cast<uintptr_t>(pCsw), 13);

            // Stalled?
            if(nResult == -Stall)
            {
                // Perform full reset and reset both pipes
                massStorageReset();
                clearEndpointHalt(m_pInEndpoint);
                clearEndpointHalt(m_pOutEndpoint);

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
                return !pCsw->nStatus;
            }
        }

        if(nResult == 13)
        {
            Csw *pCsw = reinterpret_cast<Csw*>(pRespBuffer);
            if(pCsw->nSig == CswSig)
            {
                DEBUG_LOG("USB: MSD: Early CSW with status " << pCsw->nStatus << ", residue: " << pCsw->nResidue);
                return !pCsw->nStatus;
            }
        }

        if(nResult < 0)
            return false;
    }

    Csw *pCsw = new Csw;
    PointerGuard<Csw> guard2(pCsw);
    nResult = syncIn(m_pInEndpoint, reinterpret_cast<uintptr_t>(pCsw), 13);

    /// \todo Should probably just be transaction errors and stalls
    if(nResult < 0)
    {
        if(!clearEndpointHalt(m_pInEndpoint))
        {
            massStorageReset();
            if(!clearEndpointHalt(m_pInEndpoint))
            {
                DEBUG_LOG("USB: MSD: Reading CSW ended up failing after endpoint halt cleared, and a mass storage reset, with status " << nResult);
                return false;
            }
        }
        else
        {
            nResult = syncIn(m_pInEndpoint, reinterpret_cast<uintptr_t>(pCsw), 13);
            if(nResult < 0)
            {
                DEBUG_LOG("USB: MSD: Reading CSW ended up failing after endpoint halt cleared, with status " << nResult);
                massStorageReset();
                return false;
            }
        }
    }

    DEBUG_LOG("USB: MSD: Command finished STS = " << pCsw->nStatus << " SIG=" << pCsw->nSig);

    return !pCsw->nStatus;
}


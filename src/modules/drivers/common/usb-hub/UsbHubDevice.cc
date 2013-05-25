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

#include <utilities/PointerGuard.h>
#include <usb/UsbDevice.h>
#include <usb/UsbHub.h>
#include "UsbHubDevice.h"

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

UsbHubDevice::UsbHubDevice(UsbDevice *dev) : UsbDevice(dev), UsbHub()
{
}

UsbHubDevice::~UsbHubDevice()
{
}

void UsbHubDevice::initialiseDriver()
{
    uint8_t len = getDescriptorLength(0, 0, UsbRequestType::Class);
    void *pDesc = 0;
    if(len)
    {
        pDesc = getDescriptor(0, 0, len, UsbRequestType::Class);
        if(!pDesc)
            return;
    }
    else
        return;

    HubDescriptor *pDescriptor = new HubDescriptor(pDesc);
    DEBUG_LOG("USB: HUB: Found a hub with " << Dec << pDescriptor->nPorts << Hex << " ports and hubCharacteristics = " << pDescriptor->hubCharacteristics);
    m_nPorts = pDescriptor->nPorts;
    for(size_t i = 0; i < m_nPorts; i++)
    {
        // Grab this port's status
        uint32_t portStatus = getPortStatus(i);

        // Is power on?
        if(!(portStatus & (1 << 8)))
        {
            DEBUG_LOG("USB: HUB: Powering up port " << Dec << i << Hex << " [status = " << portStatus << "]...");

            // Power it on
            setPortFeature(i, PortPower);

            // Delay while the power goes on
            delay(50);

            // Done.
            portStatus = getPortStatus(i);

            // If port power never went on, skip this port
            if(!(portStatus & (1 << 8)))
            {
                DEBUG_LOG("USB: HUB: Port " << Dec << i << Hex << " couldn't be powered up.");
                continue;
            }

            DEBUG_LOG("USB: HUB: Powered up port " << Dec << i << Hex << " [status = " << portStatus << "]...");
        }

        if(portReset(i))
        {
            // Got a device - what type?
            portStatus = getPortStatus(i);
            if(portStatus & (1 << 10))
            {
                // High-speed
                DEBUG_LOG("USB: HUB: Hub port " << Dec << i << Hex << " has a high-speed device attached to it.");
                deviceConnected(i, HighSpeed);
            }
            else if(portStatus & (1 << 9))
            {
                // Low-speed
                DEBUG_LOG("USB: HUB: Hub port " << Dec << i << Hex << " has a low-speed device attached to it.");
                deviceConnected(i, LowSpeed);
            }
            else
            {
                // Full-speed
                DEBUG_LOG("USB: HUB: Hub port " << Dec << i << Hex << " has a full-speed device attached to it.");
                deviceConnected(i, FullSpeed);
            }
        }
    }

    m_UsbState = HasDriver;
}

bool UsbHubDevice::portReset(uint8_t nPort, bool bErrorResponse)
{
    // Reset the port
    setPortFeature(nPort, PortReset);

    // Delay while the reset completes
    delay(50);
    
    // Done with reset
    clearPortFeature(nPort, PortReset);

    // Wait for completion
    while((getPortStatus(nPort) & (1 << 4)));

    // Port has been powered on and now reset, check to see if it's enabled and a device is connected
    uint32_t portStatus = getPortStatus(nPort);
    return ((portStatus & 0x3) == 0x3);
}

bool UsbHubDevice::setPortFeature(size_t port, PortFeatureSelectors feature)
{
    return controlRequest(HubPortRequest, UsbRequest::SetFeature, feature, (port + 1) & 0xFF, 0, 0);
}

bool UsbHubDevice::clearPortFeature(size_t port, PortFeatureSelectors feature)
{
    return controlRequest(HubPortRequest, UsbRequest::ClearFeature, feature, (port + 1) & 0xFF, 0, 0);
}

uint32_t UsbHubDevice::getPortStatus(size_t port)
{
    uint32_t *portStatus = new uint32_t(0);
    PointerGuard<uint32_t> guard(portStatus);
    controlRequest(UsbRequestDirection::In | HubPortRequest, UsbRequest::GetStatus, 0, (port + 1) & 0xFF, 4, reinterpret_cast<uintptr_t>(portStatus));

    return *portStatus;
}

void UsbHubDevice::addTransferToTransaction(uintptr_t pTransaction, bool bToggle, UsbPid pid, uintptr_t pBuffer, size_t nBytes)
{
    UsbHub *pParent = static_cast<UsbHub*>(UsbDevice::m_pParent);
    pParent->addTransferToTransaction(pTransaction, bToggle, pid, pBuffer, nBytes);
}

uintptr_t UsbHubDevice::createTransaction(UsbEndpoint endpointInfo)
{
    UsbHub *pParent = static_cast<UsbHub*>(UsbDevice::m_pParent);
    if((m_Speed == HighSpeed) && (endpointInfo.speed != HighSpeed) && !endpointInfo.nHubAddress)
        endpointInfo.nHubAddress = m_nAddress;
    return pParent->createTransaction(endpointInfo);
}

void UsbHubDevice::doAsync(uintptr_t pTransaction, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    UsbHub *pParent = static_cast<UsbHub*>(UsbDevice::m_pParent);
    pParent->doAsync(pTransaction, pCallback, pParam);
}

void UsbHubDevice::addInterruptInHandler(UsbEndpoint endpointInfo, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    UsbHub *pParent = static_cast<UsbHub*>(UsbDevice::m_pParent);
    if((m_Speed == HighSpeed) && (endpointInfo.speed != HighSpeed) && (!endpointInfo.nHubAddress))
        endpointInfo.nHubAddress = m_nAddress;
    pParent->addInterruptInHandler(endpointInfo, pBuffer, nBytes, pCallback, pParam);
}

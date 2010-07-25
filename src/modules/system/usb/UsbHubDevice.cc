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

#include <usb/UsbDevice.h>
#include <usb/UsbHub.h>
#include <usb/UsbHubDevice.h>

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

UsbHubDevice::UsbHubDevice(UsbDevice *dev) : Device(dev), UsbDevice(dev)
{
    HubDescriptor *pDescriptor = new HubDescriptor(getDescriptor(0, 0, getDescriptorLength(0, 0, RequestType::Class), RequestType::Class));
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

        // Reset the port
        setPortFeature(i, PortReset);

        // Delay while the reset completes
        delay(50);

        // Wait for completion
        while(true)
            if(!(getPortStatus(i) & (1 << 4)))
                break;

        // Port has been powered on and now reset, check to see if it's enabled and a device is connected
        portStatus = getPortStatus(i);
        if(portStatus & 0x3)
        {
            // Got a device - what type?
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
                // Low-speed
                DEBUG_LOG("USB: HUB: Hub port " << Dec << i << Hex << " has a full-speed device attached to it.");
                deviceConnected(i, FullSpeed);
            }
        }
        else
        {
            // Disable the port's power, no device on it
            clearPortFeature(i, PortPower);
        }
    }
}

UsbHubDevice::~UsbHubDevice()
{
}

bool UsbHubDevice::setPortFeature(size_t port, PortFeatureSelectors feature)
{
    return !controlRequest(0x23, 3, feature, port + 1, 0, 0);
}

bool UsbHubDevice::clearPortFeature(size_t port, PortFeatureSelectors feature)
{
    return !controlRequest(0x23, 1, feature, (port + 1) & 0xFF, 0, 0);
}

uint32_t UsbHubDevice::getPortStatus(size_t port)
{
    static uint32_t portStatus = 0;
    controlRequest(0xA3, 0, 0, port + 1, 4, reinterpret_cast<uintptr_t>(&portStatus));

    return portStatus;
}

void UsbHubDevice::addTransferToTransaction(uintptr_t pTransaction, bool bToggle, UsbPid pid, uintptr_t pBuffer, size_t nBytes)
{
    UsbHub *pParent = dynamic_cast<UsbHub*>(m_pParent);
    if(!pParent)
        return;
    pParent->addTransferToTransaction(pTransaction, bToggle, pid, pBuffer, nBytes);
}

uintptr_t UsbHubDevice::createTransaction(UsbEndpoint endpointInfo)
{
    UsbHub *pParent = dynamic_cast<UsbHub*>(m_pParent);
    if(!pParent)
        return static_cast<uintptr_t>(-1);
    if(m_Speed == HighSpeed && endpointInfo.speed != HighSpeed && !endpointInfo.nHubAddress)
        endpointInfo.nHubAddress = m_nAddress;
    return pParent->createTransaction(endpointInfo);
}

void UsbHubDevice::doAsync(uintptr_t pTransaction, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    UsbHub *pParent = dynamic_cast<UsbHub*>(m_pParent);
    if(!pParent)
        return;
    pParent->doAsync(pTransaction, pCallback, pParam);
}

void UsbHubDevice::addInterruptInHandler(UsbEndpoint endpointInfo, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    UsbHub *pParent = dynamic_cast<UsbHub*>(m_pParent);
    if(!pParent)
        return;
    if(m_Speed == HighSpeed && endpointInfo.speed != HighSpeed && !endpointInfo.nHubAddress)
        endpointInfo.nHubAddress = m_nAddress;
    pParent->addInterruptInHandler(endpointInfo, pBuffer, nBytes, pCallback, pParam);
}

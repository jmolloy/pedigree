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

UsbHubDevice::UsbHubDevice(UsbDevice *dev) : Device(dev), UsbDevice(dev)
{
    HubDescriptor *pDescriptor = new HubDescriptor(getDescriptor(0, 0, getDescriptorLength(0, 0, RequestType::Class), RequestType::Class));
    NOTICE("USB: HUB: Found a hub with "<<Dec<<pDescriptor->nPorts<<Hex<<" ports and hubCharacteristics="<<pDescriptor->hubCharacteristics);
    m_nPorts = pDescriptor->nPorts;
    for(size_t i=0;i<m_nPorts;i++)
    {
        uint16_t portStatus[2];
        controlRequest(0xa3, 0, 0, i+1, 4, reinterpret_cast<uintptr_t>(&portStatus));
        if(portStatus[0] & 1)
        {
            controlRequest(0x23, 3, 4, i+1);
            deviceConnected(i, LowSpeed);
        }
    }
}

UsbHubDevice::~UsbHubDevice()
{
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

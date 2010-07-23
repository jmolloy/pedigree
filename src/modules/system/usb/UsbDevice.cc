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

#include <usb/Usb.h>
#include <usb/UsbDevice.h>
#include <usb/UsbHub.h>
#include <processor/Processor.h>
#include <utilities/PointerGuard.h>

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

ssize_t UsbDevice::doSync(UsbDevice::Endpoint *pEndpoint, UsbPid pid, uintptr_t pBuffer, size_t nBytes)
{
    if(!pEndpoint)
    {
        ERROR("USB: UsbDevice::doSync called with invalid endpoint");
        return -1;
    }

    UsbHub *pParentHub = dynamic_cast<UsbHub*>(m_pParent);
    if(!pParentHub)
    {
        ERROR("USB: Orphaned UsbDevice!");
        return -1;
    }

    UsbEndpoint endpointInfo(m_nAddress, m_nPort, pEndpoint->nEndpoint, m_Speed, pEndpoint->nMaxPacketSize);
    uintptr_t nTransaction = pParentHub->createTransaction(endpointInfo);
    if(nTransaction == static_cast<uintptr_t>(-1))
    {
        ERROR("UsbDevice: couldn't get a valid transaction to work with from the parent hub");
        return -1;
    }

    pParentHub->addTransferToTransaction(nTransaction, pEndpoint->bDataToggle, pid, pBuffer, nBytes);
    /*if(nBytes > pEndpoint->nMaxPacketSize)
    {
        for(size_t i=0;i<nBytes;)
        {
            ssize_t nResult = pParentHub->doSync(endpointInfo, UsbPidIn, pBuffer+i, (nBytes-i)<pEndpoint->nMaxPacketSize?(nBytes-i):pEndpoint->nMaxPacketSize);
            if(nResult < 0)
                return nResult;
            i += nResult;
            pEndpoint->bDataToggle = !pEndpoint->bDataToggle;
        }
        return nBytes;
    }*/

    //ssize_t nResult = pParentHub->doSync(endpointInfo, nPid, pBuffer, nBytes);
    ssize_t nResult = pParentHub->doSync(nTransaction);
    pEndpoint->bDataToggle = !pEndpoint->bDataToggle;
    return nResult;
}

ssize_t UsbDevice::syncSetup(Setup *setup)
{
    return doSync(m_pEndpoints[0], UsbPidSetup, reinterpret_cast<uintptr_t>(setup), sizeof(Setup));
}

ssize_t UsbDevice::syncIn(uint8_t nEndpoint, uintptr_t pBuffer, size_t nBytes)
{
    return doSync(m_pEndpoints[nEndpoint], UsbPidIn, pBuffer, nBytes);
}

ssize_t UsbDevice::syncOut(uint8_t nEndpoint, uintptr_t pBuffer, size_t nBytes)
{
    return doSync(m_pEndpoints[nEndpoint], UsbPidOut, pBuffer, nBytes);
}

ssize_t UsbDevice::controlRequest(uint8_t nRequestType, uint8_t nRequest, uint16_t nValue, uint16_t nIndex, uint16_t nLength, uintptr_t pBuffer)
{
    // Setup structure - holds request details
    Setup *pSetup = new Setup(nRequestType, nRequest, nValue, nIndex, nLength);
    PointerGuard<Setup> guard(pSetup);

    UsbHub *pParentHub = dynamic_cast<UsbHub*>(m_pParent);
    if(!pParentHub)
    {
        ERROR("USB: Orphaned UsbDevice!");
        return -1;
    }

    UsbEndpoint endpointInfo(m_nAddress, m_nPort, 0, m_Speed, 64);

    uintptr_t nTransaction = pParentHub->createTransaction(endpointInfo);
    if(nTransaction == static_cast<uintptr_t>(-1))
    {
        ERROR("UsbDevice: couldn't get a valid transaction to work with from the parent hub");
        return -1;
    }

    // Setup Transfer - handles the SETUP phase of the transfer
    pParentHub->addTransferToTransaction(nTransaction, false, UsbPidSetup, reinterpret_cast<uintptr_t>(pSetup), sizeof(Setup));

    // Data Transfer - handles data transfer
    if(nLength)
        pParentHub->addTransferToTransaction(nTransaction, true, nRequestType & 0x80 ? UsbPidIn : UsbPidOut, pBuffer, nLength);

    // Handshake Transfer - IN when we send data to the device, OUT when we receive. Zero-length.
    pParentHub->addTransferToTransaction(nTransaction, true, nRequestType & 0x80 ? UsbPidOut : UsbPidIn, 0, 0);

    // Wait for the transaction to complete, return result
    return pParentHub->doSync(nTransaction);
}

int16_t UsbDevice::status()
{
    uint16_t nStatus;
    ssize_t nResult = controlRequest(0x80, 0, 0, 0, 2, reinterpret_cast<uintptr_t>(&nStatus));
    if(nResult < 0)
        return nResult;
    return nStatus;
}

bool UsbDevice::assignAddress(uint8_t nAddress)
{
    if(controlRequest(0, 5, nAddress, 0) < 0)
        return false;
    m_nAddress = nAddress;
    return true;
}

bool UsbDevice::useConfiguration(uint8_t nConfig)
{
    m_pConfiguration = m_pDescriptor->pConfigurations[nConfig];
    return !controlRequest(0, 9, m_pConfiguration->pDescriptor->nConfig, 0);
}

bool UsbDevice::useInterface(uint8_t nInterface)
{
    m_pInterface = m_pConfiguration->pInterfaces[nInterface];
    // Gathering endpoints
    for(size_t i = 0;i<m_pInterface->pEndpoints.count();i++)
    {
        Endpoint *pEndpoint = m_pInterface->pEndpoints[i];
        if(!m_pEndpoints[pEndpoint->nEndpoint])
            m_pEndpoints[pEndpoint->nEndpoint] = pEndpoint;
        else
        {
            // Found a multiple-purpose endpoint
            Endpoint *pOldEndpoint = m_pEndpoints[pEndpoint->nEndpoint];
            Endpoint *pNewEndpoint = new Endpoint(pOldEndpoint);
            // In case our previous endpoint was obtained from two endpoints,
            // delete it. Strange, but can happen
            if(!pOldEndpoint->pDescriptor)
                delete pOldEndpoint;
            pNewEndpoint->bIn = pNewEndpoint->bIn || pEndpoint->bIn;
            pNewEndpoint->bOut = pNewEndpoint->bOut || pEndpoint->bOut;
            pNewEndpoint->nMaxPacketSize = pNewEndpoint->nMaxPacketSize < pEndpoint->nMaxPacketSize?pNewEndpoint->nMaxPacketSize:pEndpoint->nMaxPacketSize;
            m_pEndpoints[pEndpoint->nEndpoint] = pNewEndpoint;
        }
    }
    return true;
    //return !controlRequest(0, 9, m_pConfiguration->pDescriptor->nConfig, 0);
}

void *UsbDevice::getDescriptor(uint8_t nDescriptor, uint8_t nSubDescriptor, uint16_t nBytes, uint8_t requestType)
{
    uint8_t *pBuffer = new uint8_t[nBytes];
    uint16_t nIndex = requestType & RequestRecipient::Interface?m_pInterface->pDescriptor->nInterface:0;

    /// \todo Proper language ID handling!
    if(nDescriptor == 3)
        nIndex = 0x0409; // English (US)

    if(controlRequest(0x80|requestType, 6, nDescriptor<<8|nSubDescriptor, nIndex, nBytes, reinterpret_cast<uintptr_t>(pBuffer)) < 0)
    {
        delete [] pBuffer;
        return 0;
    }
    return pBuffer;
}

uint8_t UsbDevice::getDescriptorLength(uint8_t nDescriptor, uint8_t nSubDescriptor, uint8_t requestType)
{
    uint8_t pLength;
    uint16_t nIndex = requestType & RequestRecipient::Interface?m_pInterface->pDescriptor->nInterface:0;

    /// \todo Proper language ID handling
    if(nDescriptor == 3)
        nIndex = 0x0409; // English (US)

    if(controlRequest(0x80|requestType, 6, nDescriptor<<8|nSubDescriptor, nIndex, 1, reinterpret_cast<uintptr_t>(&pLength)) < 0)
        return 0;
    return pLength;
}

char *UsbDevice::getString(uint8_t nString)
{
    if(!nString)
    {
        char *pString = new char[1];
        pString[0] = '\0';
        return pString;
    }

    uint8_t descriptorLength = getDescriptorLength(3, nString);
    char *pBuffer = static_cast<char*>(getDescriptor(3, nString, descriptorLength));
    if(pBuffer)
    {
        size_t nStrLength = (descriptorLength - 2) / 2;
        char *pString = new char[nStrLength+1];
        for(size_t i=0;i<nStrLength;i++)
            pString[i] = pBuffer[2+i*2];
        pString[nStrLength] = 0;
        delete pBuffer;
        return pString;
    }
    return 0;
}

void UsbDevice::populateDescriptors()
{
    m_pDescriptor = new DeviceDescriptor(getDescriptor(1, 0, getDescriptorLength(1, 0)));
    if(!m_pDescriptor)
        return; /// \todo Better error handling
    
    // Debug dump of the device descriptor
    DEBUG_LOG("USB version: " << Dec << (m_pDescriptor->pDescriptor->nBcdUsbRelease >> 8) << "." << (m_pDescriptor->pDescriptor->nBcdUsbRelease & 0xFF) << ".");
    DEBUG_LOG("Device class/subclass/protocol: " << m_pDescriptor->pDescriptor->nClass << "/" << m_pDescriptor->pDescriptor->nSubclass << "/" << m_pDescriptor->pDescriptor->nProtocol);
    DEBUG_LOG("Maximum control packet size is " << Dec << m_pDescriptor->pDescriptor->nMaxPacketSize << Hex << " bytes.");
    DEBUG_LOG("Vendor and product IDs: " << m_pDescriptor->pDescriptor->nVendorId << ":" << m_pDescriptor->pDescriptor->nProductId << ".");
    DEBUG_LOG("Device version: " << Dec << (m_pDescriptor->pDescriptor->nBcdDeviceRelease >> 8) << "." << (m_pDescriptor->pDescriptor->nBcdDeviceRelease & 0xFF) << ".");
    DEBUG_LOG("Device has " << Dec << m_pDescriptor->pDescriptor->nConfigurations << Hex << " configurations");

    if(m_pDescriptor->pDescriptor->nVendorString)
        m_pDescriptor->sVendor = getString(m_pDescriptor->pDescriptor->nVendorString);
    else
        m_pDescriptor->sVendor = "";
    if(m_pDescriptor->pDescriptor->nProductString)
        m_pDescriptor->sProduct = getString(m_pDescriptor->pDescriptor->nProductString);
    else
        m_pDescriptor->sProduct = "";
    if(m_pDescriptor->pDescriptor->nSerialString)
        m_pDescriptor->sSerial = getString(m_pDescriptor->pDescriptor->nSerialString);
    else
        m_pDescriptor->sSerial = "";
    for(size_t i = 0;i < m_pDescriptor->pDescriptor->nConfigurations;i++)
    {
        uint16_t *pPartialConfig = static_cast<uint16_t*>(getDescriptor(2, i, 4));
        uint16_t configLength = pPartialConfig[1];
        delete pPartialConfig;
        ConfigDescriptor *pConfig = new ConfigDescriptor(getDescriptor(2, i, configLength), configLength);
        pConfig->sString = getString(pConfig->pDescriptor->nString);
        for(size_t j = 0;j < pConfig->pInterfaces.count();j++)
        {
            Interface *pInterface = pConfig->pInterfaces[j];
            pInterface->sString = getString(pInterface->pDescriptor->nString);
        }
        m_pDescriptor->pConfigurations.pushBack(pConfig);
    }
}

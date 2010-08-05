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

    if(!nBytes)
        return 0;

    if(nBytes && (pBuffer & 0xF))
    {
        ERROR("USB: Input pointer wasn't properly aligned [" << pBuffer << ", " << nBytes << "]");
        return -1;
    }

    ssize_t nResult = 0;

    UsbEndpoint endpointInfo(m_nAddress, m_nPort, pEndpoint->nEndpoint, m_Speed, pEndpoint->nMaxPacketSize);
    uintptr_t nTransaction = pParentHub->createTransaction(endpointInfo);
    if(nTransaction == static_cast<uintptr_t>(-1))
    {
        ERROR("UsbDevice: couldn't get a valid transaction to work with from the parent hub");
        return -1;
    }

    size_t byteOffset = 0;
    while(nBytes)
    {
        size_t nBytesThisTransaction = nBytes > pEndpoint->nMaxPacketSize ? pEndpoint->nMaxPacketSize : nBytes;

        pParentHub->addTransferToTransaction(nTransaction, pEndpoint->bDataToggle, pid, pBuffer + byteOffset, nBytesThisTransaction);
        byteOffset += nBytesThisTransaction;
        nBytes -= nBytesThisTransaction;

        pEndpoint->bDataToggle = !pEndpoint->bDataToggle;
    }

    nResult = pParentHub->doSync(nTransaction);
    return nResult;
}

ssize_t UsbDevice::syncIn(Endpoint *pEndpoint, uintptr_t pBuffer, size_t nBytes)
{
    return doSync(pEndpoint, UsbPidIn, pBuffer, nBytes);
}

ssize_t UsbDevice::syncOut(Endpoint *pEndpoint, uintptr_t pBuffer, size_t nBytes)
{
    return doSync(pEndpoint, UsbPidOut, pBuffer, nBytes);
}

bool UsbDevice::controlRequest(uint8_t nRequestType, uint8_t nRequest, uint16_t nValue, uint16_t nIndex, uint16_t nLength, uintptr_t pBuffer)
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
        pParentHub->addTransferToTransaction(nTransaction, true, nRequestType & RequestDirection::In ? UsbPidIn : UsbPidOut, pBuffer, nLength);

    // Handshake Transfer - IN when we send data to the device, OUT when we receive. Zero-length.
    pParentHub->addTransferToTransaction(nTransaction, true, nRequestType & RequestDirection::In ? UsbPidOut : UsbPidIn, 0, 0);

    // Wait for the transaction to complete
    ssize_t nResult = pParentHub->doSync(nTransaction);
    // Return false if we had an error, true otherwise
    return nResult >= 0; // >= sizeof(Setup) + nLength;
}

uint16_t UsbDevice::getStatus()
{
    uint16_t *nStatus = new uint16_t(0);
    PointerGuard<uint16_t> guard(nStatus);
    controlRequest(RequestDirection::In, Request::GetStatus, 0, 0, 2, reinterpret_cast<uintptr_t>(nStatus));
    return *nStatus;
}

bool UsbDevice::clearEndpointHalt(Endpoint *pEndpoint)
{
    return controlRequest(RequestRecipient::Endpoint, Request::ClearFeature, 0, pEndpoint->nEndpoint);
}

bool UsbDevice::assignAddress(uint8_t nAddress)
{
    if(!controlRequest(0, Request::SetAddress, nAddress, 0))
        return false;
    m_nAddress = nAddress;
    return true;
}

bool UsbDevice::useConfiguration(uint8_t nConfig)
{
    m_pConfiguration = m_pDescriptor->pConfigurations[nConfig];
    return controlRequest(0, Request::SetConfiguration, m_pConfiguration->pDescriptor->nConfig, 0);
}

bool UsbDevice::useInterface(uint8_t nInterface)
{
    m_pInterface = m_pConfiguration->pInterfaces[nInterface];
    return true;
    //return controlRequest(0, Request::SetInterface, m_pInterface->pDescriptor->nAlternateSetting, 0);
}

void *UsbDevice::getDescriptor(uint8_t nDescriptor, uint8_t nSubDescriptor, uint16_t nBytes, uint8_t requestType)
{
    uint8_t *pBuffer = new uint8_t[nBytes];
    uint16_t nIndex = requestType & RequestRecipient::Interface ? m_pInterface->pDescriptor->nInterface : 0;

    /// \todo Proper language ID handling!
    if(nDescriptor == Descriptor::String)
        nIndex = 0x0409; // English (US)

    if(!controlRequest(RequestDirection::In | requestType, Request::GetDescriptor, (nDescriptor << 8) | nSubDescriptor, nIndex, nBytes, reinterpret_cast<uintptr_t>(pBuffer)))
    {
        delete [] pBuffer;
        return 0;
    }
    return pBuffer;
}

uint8_t UsbDevice::getDescriptorLength(uint8_t nDescriptor, uint8_t nSubDescriptor, uint8_t requestType)
{
    uint8_t *length = new uint8_t(0);
    PointerGuard<uint8_t> guard(length);
    uint16_t nIndex = requestType & RequestRecipient::Interface ? m_pInterface->pDescriptor->nInterface : 0;

    /// \todo Proper language ID handling
    if(nDescriptor == Descriptor::String)
        nIndex = 0x0409; // English (US)

    controlRequest(RequestDirection::In | requestType, Request::GetDescriptor, (nDescriptor << 8) | nSubDescriptor, nIndex, 1, reinterpret_cast<uintptr_t>(length));
    return *length;
}

String UsbDevice::getString(uint8_t nString)
{
    // A value of zero means there's no string
    if(!nString)
        return String("");

    uint8_t descriptorLength = getDescriptorLength(Descriptor::String, nString);
    if(!descriptorLength)
        return String("");

    char *pBuffer = static_cast<char*>(getDescriptor(Descriptor::String, nString, descriptorLength));
    if(!pBuffer)
        return String("");

    // Get the number of characters in the string and allocate a new buffer for the string
    size_t nStrLength = (descriptorLength - 2) / 2;
    char *pString = new char[nStrLength + 1];

    // For each character, get the lower part of the UTF-16 value
    /// \todo UTF-8 support of some kind
    for(size_t i = 0;i < nStrLength;i++)
        pString[i] = pBuffer[2 + i*2];

    // Set the last byte of the string to 0, delete the old buffer and return the string
    pString[nStrLength] = 0;
    delete pBuffer;
    return String(pString);
}

void UsbDevice::populateDescriptors()
{
    m_pDescriptor = new DeviceDescriptor(getDescriptor(Descriptor::Device, 0, getDescriptorLength(Descriptor::Device, 0)));
    if(!m_pDescriptor)
        return; /// \todo Better error handling

    // Debug dump of the device descriptor
#ifdef USB_VERBOSE_DEBUG
    DEBUG_LOG("USB version: " << Dec << (m_pDescriptor->pDescriptor->nBcdUsbRelease >> 8) << "." << (m_pDescriptor->pDescriptor->nBcdUsbRelease & 0xFF) << ".");
    DEBUG_LOG("Device class/subclass/protocol: " << m_pDescriptor->pDescriptor->nClass << "/" << m_pDescriptor->pDescriptor->nSubclass << "/" << m_pDescriptor->pDescriptor->nProtocol);
    DEBUG_LOG("Maximum control packet size is " << Dec << m_pDescriptor->pDescriptor->nMaxPacketSize << Hex << " bytes.");
    DEBUG_LOG("Vendor and product IDs: " << m_pDescriptor->pDescriptor->nVendorId << ":" << m_pDescriptor->pDescriptor->nProductId << ".");
    DEBUG_LOG("Device version: " << Dec << (m_pDescriptor->pDescriptor->nBcdDeviceRelease >> 8) << "." << (m_pDescriptor->pDescriptor->nBcdDeviceRelease & 0xFF) << ".");
    DEBUG_LOG("Device has " << Dec << m_pDescriptor->pDescriptor->nConfigurations << Hex << " configurations");
#endif

    // Descriptor number for the configuration descriptor
    uint8_t nConfigDescriptor = Descriptor::Configuration;

    // Handle high-speed capable devices running at full-speed:
    // If the device works at full-speed and it has a device qualifier descriptor,
    // it means that the normal configuration descriptor is for high-speed,
    // and we need to use the other speed configuration descriptor
    if(m_Speed == FullSpeed && getDescriptorLength(Descriptor::DeviceQualifier, 0))
        nConfigDescriptor = Descriptor::OtherSpeedConfiguration;

    // Get the vendor, product and serial strings
    m_pDescriptor->sVendor = getString(m_pDescriptor->pDescriptor->nVendorString);
    m_pDescriptor->sProduct = getString(m_pDescriptor->pDescriptor->nProductString);
    m_pDescriptor->sSerial = getString(m_pDescriptor->pDescriptor->nSerialString);

    // Grab each configuration from the device
    for(size_t i = 0; i < m_pDescriptor->pDescriptor->nConfigurations; i++)
    {
        // Get the total size of this configuration descriptor
        uint16_t *pPartialConfig = static_cast<uint16_t*>(getDescriptor(nConfigDescriptor, i, 4));
        uint16_t configLength = pPartialConfig[1];
        delete pPartialConfig;

        ConfigDescriptor *pConfig = new ConfigDescriptor(getDescriptor(nConfigDescriptor, i, configLength), configLength);
        pConfig->sString = getString(pConfig->pDescriptor->nString);
        for(size_t j = 0; j < pConfig->pInterfaces.count(); j++)
        {
            Interface *pInterface = pConfig->pInterfaces[j];
            pInterface->sString = getString(pInterface->pDescriptor->nString);
        }
        m_pDescriptor->pConfigurations.pushBack(pConfig);
    }
}

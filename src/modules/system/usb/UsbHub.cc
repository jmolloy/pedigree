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

#include <processor/Processor.h>
#include <usb/UsbDevice.h>
#include <usb/UsbHub.h>
#include <usb/UsbHubDevice.h>
#include <usb/UsbHumanInterfaceDevice.h>
#include <usb/UsbMassStorageDevice.h>
#include <usb/FtdiSerialDevice.h>
#include <utilities/ExtensibleBitmap.h>
#include <LockGuard.h>

void UsbHub::deviceConnected(uint8_t nPort)
{
    //NOTICE("USB: Adding device on port "<<Dec<<nPort<<Hex<<"...");
    // Create a bitmap to hold the used addresses
    ExtensibleBitmap *pUsedAddresses = new ExtensibleBitmap();
    pUsedAddresses->set(0);
    // Find the root hub
    UsbHub *pRootHub = this;
    while(dynamic_cast<UsbHub*>(pRootHub->getParent()))
        pRootHub = dynamic_cast<UsbHub*>(pRootHub->getParent());
    // Fill in the used addresses
    pRootHub->getUsedAddresses(pUsedAddresses);
    // Get first unused address and check it
    uint8_t nAddress = pUsedAddresses->getFirstClear();
    if(nAddress > 127)
        FATAL("USB: HUB: Out of addresses!");

    // Create the UsbDevice instance and set us as parent
    UsbDevice *pDevice = new UsbDevice();
    pDevice->setParent(this);
    // Assign the address we've chosen
    pDevice->assignAddress(nAddress);
    // Set port number
    pDevice->setPort(nPort);
    // Get all descriptors in place
    pDevice->populateDescriptors();
    UsbDevice::DeviceDescriptor *pDes = pDevice->getDescriptor();
    // Currently we only support the default configuration
    if(pDes->pConfigurations.count() > 1)
        NOTICE("USB: TODO: multiple configuration devices");
    pDevice->useConfiguration(0);
    Vector<UsbDevice::Interface*> pInterfaces = pDevice->getConfiguration()->pInterfaces;
    for(size_t i = 0;i<pInterfaces.count();i++)
    {
        UsbDevice::Interface *pInterface = pInterfaces[i];
        // Skip alternate settings
        if(pInterface->pDescriptor->nAlternateSetting)
            continue;
        // Just in case we have a single-interface device with the class code(s) in the device descriptor
        if(pInterfaces.count() == 1 && pDes->pDescriptor->nClass && !pInterface->pDescriptor->nClass)
        {
            pInterface->pDescriptor->nClass = pDes->pDescriptor->nClass;
            pInterface->pDescriptor->nSubclass = pDes->pDescriptor->nSubclass;
            pInterface->pDescriptor->nProtocol = pDes->pDescriptor->nProtocol;
        }
        // If we're not at the first interface, we have to clone the UsbDevice
        else if(i)
        {
            pDevice = new UsbDevice(pDevice);
            pDevice->setParent(this);
        }
        // Set the right interface
        pDevice->useInterface(i);
        NOTICE("USB: Device: "<<pDes->sVendor<<" "<<pDes->sProduct<<", class "<<Dec<<pInterface->pDescriptor->nClass<<":"<<pInterface->pDescriptor->nSubclass<<":"<<pInterface->pDescriptor->nProtocol<<Hex);
        // TODO: make this a bit more general... harcoding some numbers and some class names doesn't sound good :|
        addChild(pDevice);
        if(pInterface->pDescriptor->nClass == 9)
            replaceChild(pDevice, new UsbHubDevice(pDevice));
        else if(pInterface->pDescriptor->nClass == 3)
            replaceChild(pDevice, new UsbHumanInterfaceDevice(pDevice));
        else if(pInterface->pDescriptor->nClass == 8)
            replaceChild(pDevice, new UsbMassStorageDevice(pDevice));
        else if(pDes->pDescriptor->nVendorId == 0x0403 && pDes->pDescriptor->nProductId == 0x6001)
            replaceChild(pDevice, new FtdiSerialDevice(pDevice));
        return;
    }
}

void UsbHub::deviceDisconnected(uint8_t nPort)
{
    for (size_t i = 0;i < m_Children.count();i++)
    {
        UsbDevice *pDevice = dynamic_cast<UsbDevice*>(m_Children[i]);
        if(pDevice && pDevice->getPort() == nPort)
            delete pDevice;
    }
}

void UsbHub::getUsedAddresses(ExtensibleBitmap *pBitmap)
{
    for (size_t i = 0;i < m_Children.count();i++)
    {
        UsbDevice *pDevice = dynamic_cast<UsbDevice*>(m_Children[i]);
        if(pDevice)
            pBitmap->set(pDevice->getAddress());
        UsbHub *pHub = dynamic_cast<UsbHub*>(m_Children[i]);
        if(pHub)
            pHub->getUsedAddresses(pBitmap);
    }
}

void UsbHub::syncCallback(uintptr_t pParam, ssize_t ret)
{
    if(!pParam)
        return;
    UsbHub *pController = reinterpret_cast<UsbHub*>(pParam);
    pController->m_SyncRet = ret;
    pController->m_SyncSemaphore.release();
}

ssize_t UsbHub::doSync(uint8_t nAddress, uint8_t nEndpoint, uint8_t nPid, uintptr_t pBuffer, size_t nBytes, uint32_t timeout)
{
    LockGuard<Mutex> guard(m_SyncMutex);
    doAsync(nAddress, nEndpoint, nPid, pBuffer, nBytes, syncCallback, reinterpret_cast<uintptr_t>(static_cast<UsbHub*>(this)));
    m_SyncSemaphore.acquire(1);/*, 0, timeout * 1000);
    if(Processor::information().getCurrentThread()->wasInterrupted())
        return -1;
    else
        */
    return m_SyncRet;
}

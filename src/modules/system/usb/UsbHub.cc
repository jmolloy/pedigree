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
#include <usb/Usb.h>
#include <usb/UsbDevice.h>
#include <usb/UsbHub.h>
#include <usb/UsbHubDevice.h>
#include <usb/UsbHumanInterfaceDevice.h>
#include <usb/UsbMassStorageDevice.h>
#include <usb/FtdiSerialDevice.h>
#include <utilities/ExtensibleBitmap.h>
#include <LockGuard.h>

#define delay(n) do{Semaphore semWAIT(0);semWAIT.acquire(1, 0, n*1000);}while(0)

bool UsbHub::deviceConnected(uint8_t nPort, UsbSpeed speed)
{
    NOTICE("USB: Adding device on port " << Dec << nPort << Hex);
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
    {
        ERROR("USB: HUB: Out of addresses!");
        return false;
    }

    // Create the UsbDevice instance and set us as parent
    UsbDevice *pDevice = new UsbDevice();
    pDevice->setParent(this);
    // Set port number
    pDevice->setPort(nPort);
    // Set speed
    pDevice->setSpeed(speed);
    // Assign the address we've chosen
    if(!pDevice->assignAddress(nAddress))
    {
        ERROR("USB: HUB: address assignment failed!");
        return false;
    }


    // Get all descriptors in place
    pDevice->populateDescriptors();
    UsbDevice::DeviceDescriptor *pDes = pDevice->getDescriptor();
    // Currently we only support the default configuration
    if(pDes->pConfigurations.count() > 1)
        DEBUG_LOG("USB: TODO: multiple configuration devices");
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
        NOTICE("USB: Device: " << pDes->sVendor << " " << pDes->sProduct << ", class " << Dec << pInterface->pDescriptor->nClass << ":" << pInterface->pDescriptor->nSubclass << ":" << pInterface->pDescriptor->nProtocol << Hex);
        /// \todo Make this a bit more general. Harcoding some numbers and some class names doesn't sound good
        addChild(pDevice);
        if(pInterface->pDescriptor->nClass == 9)
            replaceChild(pDevice, new UsbHubDevice(pDevice));
        else if(pInterface->pDescriptor->nClass == 3)
        {
            replaceChild(pDevice, new UsbHumanInterfaceDevice(pDevice));
            /// \bug HID devices with interface number > 0 can cause problems
            return true;
        }
        else if(pInterface->pDescriptor->nClass == 8)
            replaceChild(pDevice, new UsbMassStorageDevice(pDevice));
        else if(pDes->pDescriptor->nVendorId == 0x0403 && pDes->pDescriptor->nProductId == 0x6001)
            replaceChild(pDevice, new FtdiSerialDevice(pDevice));
    }
    return true;
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

void UsbHub::syncCallback(uintptr_t pParam, ssize_t nResult)
{
    if(!pParam)
        return;
    SyncParam *pSyncParam = reinterpret_cast<SyncParam*>(pParam);
    pSyncParam->nResult = nResult;
    pSyncParam->semaphore.release();
}


ssize_t UsbHub::doSync(uintptr_t nTransaction, uint32_t timeout)
{
    // Create a structure to hold the semaphore and the result
    SyncParam *pSyncParam = new SyncParam();
    // Send the async request
    doAsync(nTransaction, syncCallback, reinterpret_cast<uintptr_t>(pSyncParam));
    // Wait for the semaphore to release
    pSyncParam->semaphore.acquire(1); // , timeout / 1000);
    // Delete our structure and return the result
    delete pSyncParam;
    if(Processor::information().getCurrentThread()->wasInterrupted())
        return -0xbeef;
    else
        return pSyncParam->nResult;
}
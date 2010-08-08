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
#include <utilities/ExtensibleBitmap.h>
#include <utilities/PointerGuard.h>
#include <LockGuard.h>
#include <usb/Usb.h>
#include <usb/UsbDevice.h>
#include <usb/UsbHub.h>
#include <usb/UsbPnP.h>

bool UsbHub::deviceConnected(uint8_t nPort, UsbSpeed speed)
{
    NOTICE("USB: Adding device on port " << Dec << nPort << Hex);

    // Find the root hub
    UsbHub *pRootHub = this;
    while(dynamic_cast<UsbHub*>(pRootHub->getParent()))
        pRootHub = dynamic_cast<UsbHub*>(pRootHub->getParent());

    // Get first unused address and check it
    uint8_t nAddress = pRootHub->m_UsedAddresses.getFirstClear();
    if(nAddress > 127)
    {
        ERROR("USB: HUB: Out of addresses!");
        return false;
    }

    // This address is now used
    pRootHub->m_UsedAddresses.set(nAddress);

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
        delete(pDevice);
        return false;
    }

    // Get all descriptors in place
    pDevice->populateDescriptors();
    UsbDevice::DeviceDescriptor *pDes = pDevice->getDescriptor();

    // Currently we only support the default configuration
    if(pDes->pConfigurations.count() > 1)
        DEBUG_LOG("USB: TODO: multiple configuration devices");
    pDevice->useConfiguration(0);

    // Iterate all interfaces
    Vector<UsbDevice::Interface*> pInterfaces = pDevice->getConfiguration()->pInterfaces;
    for(size_t i = 0; i < pInterfaces.count(); i++)
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

        // Add the device as a child
        addChild(pDevice);

        NOTICE("USB: Device: " << pDes->sVendor << " " << pDes->sProduct << ", class " << Dec << pInterface->pDescriptor->nClass << ":" << pInterface->pDescriptor->nSubclass << ":" << pInterface->pDescriptor->nProtocol << Hex);

        // Send it to the USB PnP manager
        UsbPnP::instance().probeDevice(pDevice);
    }
    return true;
}

void UsbHub::deviceDisconnected(uint8_t nPort)
{
    uint8_t nAddress = 0;
    for(size_t i = 0; i < m_Children.count(); i++)
    {
        UsbDevice *pDevice = dynamic_cast<UsbDevice*>(m_Children[i]);
        if(pDevice && pDevice->getPort() == nPort)
        {
            if(!nAddress)
                nAddress = pDevice->getAddress();
            else if(nAddress != pDevice->getAddress())
                ERROR("USB: HUB: Found devices on the same port with different addresses");
            delete pDevice;
        }
    }

    if(!nAddress)
        return;

    // Find the root hub
    UsbHub *pRootHub = this;
    while(dynamic_cast<UsbHub*>(pRootHub->getParent()))
        pRootHub = dynamic_cast<UsbHub*>(pRootHub->getParent());

    // This address is now free
    pRootHub->m_UsedAddresses.clear(nAddress);
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
    PointerGuard<SyncParam> guard(pSyncParam);
    // Send the async request
    doAsync(nTransaction, syncCallback, reinterpret_cast<uintptr_t>(pSyncParam));
    // Wait for the semaphore to release
    /// \bug Transaction is never deleted here - which makes pParam in the syncCallback above
    ///      invalid, causing an assertion failure in Semaphore if this times out.
    bool bTimeout = !pSyncParam->semaphore.acquire(1, timeout / 1000, (timeout % 1000) * 1000);
    // Return the result
    if(bTimeout)
        return -TransactionError;
    else
        return pSyncParam->nResult;
}

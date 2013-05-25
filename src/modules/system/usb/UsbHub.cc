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
    while(pRootHub->getParent()->getType() == Device::UsbController)
        pRootHub = static_cast<UsbHub*>(pRootHub->getParent());
    
    size_t nRetry = 0;
    uint8_t lastAddress = 0, nAddress = 0;
    
    pRootHub->ignoreConnectionChanges(nPort);
    
    // Try twice with two different addresses
    UsbDevice *pDevice = 0;
    while(nRetry < 2)
    {
        // Get first unused address and check it
        nAddress = pRootHub->m_UsedAddresses.getFirstClear();
        if(nAddress > 127)
        {
            ERROR("USB: HUB: Out of addresses!");
            return false;
        }

        // This address is now used
        pRootHub->m_UsedAddresses.set(nAddress);
        if(lastAddress)
            pRootHub->m_UsedAddresses.clear(lastAddress);
        NOTICE("USB: Allocated device on port " << Dec << nPort << Hex << " address " << nAddress);

        // Create the UsbDevice instance and set us as parent
        pDevice = new UsbDevice(nPort, speed);
        pDevice->setParent(this);

        // Initialise the device - it basically sets the address and gets the descriptors
        pDevice->initialise(nAddress);

        // Check for initialisation failures
        if(pDevice->getUsbState() != UsbDevice::Configured)
        {
            NOTICE("USB: Device initialisation ended up not giving a configured device [retry " << nRetry << " of 2].");
            
            // Cleanup descriptors
            if(pDevice->getUsbState() >= UsbDevice::HasDescriptors)
                delete pDevice->getDescriptor();

            delete pDevice;
            
            // Reset the port that this device is attached to.
            NOTICE("USB: Performing a port reset on port " << nPort);
            if((!pRootHub->portReset(nPort, true)) && (nRetry < 1))
            {
                // Give up completely
                NOTICE("USB: Port reset failed (port " << nPort << ")");
                pRootHub->ignoreConnectionChanges(nPort, false);
                pRootHub->m_UsedAddresses.clear(nAddress);
                return false;
            }
        }
        else
        {
            NOTICE("USB: Device on port " << Dec << nPort << Hex << " accepted address " << nAddress);
            break;
        }
        
        lastAddress = nAddress;
        nRetry++;
    }
    
    pRootHub->ignoreConnectionChanges(nPort, false);
    
    if(nRetry == 2)
    {
        NOTICE("Device initialisation couldn't configure the device.");
        return false;
    }

    // Get the device descriptor
    UsbDevice::DeviceDescriptor *pDescriptor = pDevice->getDescriptor();

    // Iterate all interfaces
    Vector<UsbDevice::Interface*> interfaceList = pDevice->getConfiguration()->interfaceList;
    for(size_t i = 0; i < interfaceList.count(); i++)
    {
        UsbDevice::Interface *pInterface = interfaceList[i];

        // Skip alternate settings
        if(pInterface->nAlternateSetting)
            continue;

        // If we're not at the first interface, we have to clone the UsbDevice
        if(i)
            pDevice = new UsbDevice(pDevice);

        // Set the right interface
        pDevice->useInterface(i);

        // Add the device as a child
        addChild(pDevice);

        NOTICE("USB: Device (address " << nAddress << "): " << pDescriptor->sVendor << " " << pDescriptor->sProduct << ", class " << Dec << pInterface->nClass << ":" << pInterface->nSubclass << ":" << pInterface->nProtocol << Hex);

        // Send it to the USB PnP manager
        NOTICE("pnp instance is: " << reinterpret_cast<uintptr_t>(&UsbPnP::instance()) << ".");
        // UsbPnP::instance().probeDevice(pDevice);
    }
    return true;
}

void UsbHub::deviceDisconnected(uint8_t nPort)
{
    uint8_t nAddress = 0;
    UsbDevice::DeviceDescriptor *pDescriptor = 0;

    for(size_t i = 0; i < m_Children.count(); i++)
    {
        UsbDevice *pDevice = static_cast<UsbDevice*>(m_Children[i]);

        if(!pDevice)
            continue;

        if(pDevice->getPort() != nPort)
            continue;

        if(!nAddress)
            nAddress = pDevice->getAddress();
        else if(nAddress != pDevice->getAddress())
            ERROR("USB: HUB: Found devices on the same port with different addresses");

        if(pDevice->getUsbState() >= UsbDevice::HasDescriptors)
        {
            if(!pDescriptor)
                pDescriptor = pDevice->getDescriptor();
            else if(pDescriptor != pDevice->getDescriptor())
                ERROR("USB: HUB: Found devices on the same port with different device descriptors");
        }

        delete pDevice;
    }

    if(pDescriptor)
        delete pDescriptor;

    if(!nAddress)
        return;

    // Find the root hub
    UsbHub *pRootHub = this;
    while(pRootHub->getParent()->getType() == Device::UsbController)
        pRootHub = static_cast<UsbHub*>(pRootHub->getParent());

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
    
    if(pSyncParam->timedOut)
        delete pSyncParam;
}

ssize_t UsbHub::doSync(uintptr_t nTransaction, uint32_t timeout)
{
    // Create a structure to hold the semaphore and the result
    SyncParam *pSyncParam = new SyncParam();
    
    // Send the async request
    doAsync(nTransaction, syncCallback, reinterpret_cast<uintptr_t>(pSyncParam));
    // Wait for the semaphore to release
    /// \bug Transaction is never deleted here - which makes pParam in the syncCallback above
    ///      invalid, causing an assertion failure in Semaphore if this times out.
    bool bTimeout = !pSyncParam->semaphore.acquire(1, timeout / 1000, (timeout % 1000) * 1000);
    // Return the result
    if(bTimeout)
    {
        WARNING("USB: a transaction timed out.");
        pSyncParam->timedOut = true;
        return -TransactionError;
    }
    else
    {
        ssize_t ret = pSyncParam->nResult;
        delete pSyncParam;
        return ret;
    }
}

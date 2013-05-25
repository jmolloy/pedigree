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
#include <usb/UsbPnP.h>
#include <usb/UsbDevice.h>

UsbPnP UsbPnP::m_Instance;

bool UsbPnP::probeDevice(UsbDevice *pDevice)
{
    // Is this device already handled by a driver?
    if(pDevice->getUsbState() == UsbDevice::HasDriver)
        return false;
    else if(!m_Callbacks.count())
        return false;

    UsbDevice::DeviceDescriptor *pDes = pDevice->getDescriptor();
    UsbDevice::Interface *pIface = pDevice->getInterface();

    for(List<CallbackItem*>::Iterator it = m_Callbacks.begin();
        it != m_Callbacks.end();
        it++)
    {
        CallbackItem *item = *it;
        if(!item)
            continue;

        if((item->nVendorId != VendorIdNone) && (item->nVendorId != pDes->nVendorId))
            continue;
        if((item->nProductId != ProductIdNone) && (item->nProductId != pDes->nProductId))
            continue;
        if((item->nClass != ClassNone) && (item->nClass != pIface->nClass))
            continue;
        if((item->nSubclass != SubclassNone) && (item->nSubclass != pIface->nSubclass))
            continue;
        if((item->nProtocol != ProtocolNone) && (item->nProtocol != pIface->nProtocol))
            continue;

        // Call the callback, which will give us (hopefully) a copy of pDevice, in the form of a driver class
        UsbDevice *pNewDevice = item->callback(pDevice);

        // Was this device rejected by the driver?
        if(!pNewDevice)
            continue;

        // Initialise the driver
        pNewDevice->initialiseDriver();

        // Did the device go into the driver state?
        if(pNewDevice->getUsbState() == UsbDevice::HasDriver)
        {
            // Replace the old device with the new one, delete the old one and return true
            pDevice->getParent()->replaceChild(pDevice, pNewDevice);
            delete pDevice;
            return true;
        }
        else
            delete pNewDevice;
    }
    return false;
}

void UsbPnP::reprobeDevices(Device *pParent)
{
    for(size_t i = 0; i < pParent->getNumChildren(); i++)
    {
        UsbDevice *pDevice = static_cast<UsbDevice*>(pParent->getChild(i));
        if(pDevice &&
            ((pDevice->getType() == Device::UsbGeneric) ||
            (pDevice->getType() == Device::UsbController)))
            probeDevice(pDevice);

        reprobeDevices(pParent->getChild(i));
    }
}

void UsbPnP::registerCallback(uint16_t nVendorId, uint16_t nProductId, callback_t callback)
{
    CallbackItem *item = new CallbackItem;
    item->callback = callback;
    item->nVendorId = nVendorId;
    item->nProductId = nProductId;
    item->nClass = ClassNone;
    item->nSubclass = SubclassNone;
    item->nProtocol = ProtocolNone;

    m_Callbacks.pushBack(item);

    reprobeDevices(&Device::root());
}

void UsbPnP::registerCallback(uint8_t nClass, uint8_t nSubclass, uint8_t nProtocol, callback_t callback)
{
    CallbackItem *item = new CallbackItem;
    item->callback = callback;
    item->nVendorId = VendorIdNone;
    item->nProductId = ProductIdNone;
    item->nClass = nClass;
    item->nSubclass = nSubclass;
    item->nProtocol = nProtocol;

    m_Callbacks.pushBack(item);

    reprobeDevices(&Device::root());
}

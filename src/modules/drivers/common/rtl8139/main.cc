/*
* Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin, Eduard Burtescu
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

#include <Module.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <machine/Device.h>
#include <machine/Network.h>
#include "Rtl8139.h"
#include <Log.h>

void probeDevice(Network *pDev)
{
    // Create a new RTL8139 node
    Rtl8139 *pRtl8139 = new Rtl8139(pDev);

    // Replace pDev with pRtl8139.
    pRtl8139->setParent(pDev->getParent());
    pDev->getParent()->replaceChild(pDev, pRtl8139);


    // And delete pDev for good measure.
    //  - Deletion not needed now that AtaController(pDev) destroys pDev. See Device::Device(Device *)
    //delete pDev;
}

void searchNode(Device *pDev)
{
    for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
    {
        Device *pChild = pDev->getChild(i);

        if((pChild->getPciVendorId() == RTL8139_VENDOR_ID) && (pChild->getPciDeviceId() == RTL8139_DEVICE_ID))
        {
            uintptr_t irq = pChild->getInterruptNumber();
            WARNING("RTL8139 found, IRQ = " << irq << ".");

            if(pChild->addresses()[0]->m_IsIoSpace)
                probeDevice(reinterpret_cast<Network*>(pChild));
        }

        // Recurse.
        searchNode(pChild);
    }
}

void entry()
{
    // Walk the device tree looking for controllers that have
    // "control" and "command" addresses.
    Device *pDev = &Device::root();
    searchNode(pDev);

    /// \todo ISA probe for devices
}

void exit()
{

}

MODULE_NAME("rtl8139");
MODULE_ENTRY(&entry);
MODULE_EXIT(&exit);
MODULE_DEPENDS("NetworkStack");

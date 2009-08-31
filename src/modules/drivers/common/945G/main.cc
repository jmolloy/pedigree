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
#include "945G.h"
#include <Log.h>

void probeDevice(Device *pDev)
{
    // Create a new 945G node
    i945G *p945G = new i945G(pDev);

    // Replace pDev with p945G.
    p945G->setParent(pDev->getParent());
    pDev->getParent()->replaceChild(pDev, p945G);
}

void searchNode(Device *pDev)
{
    for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
    {
        Device *pChild = pDev->getChild(i);

        if((pChild->getPciVendorId() == i945G_VENDOR_ID) && (pChild->getPciDeviceId() == i945G_DEVICE_ID))
        {
            uintptr_t irq = pChild->getInterruptNumber();
            WARNING("945G found, IRQ = " << irq << ".");

            //if(pChild->addresses()[0]->m_IsIoSpace)
                probeDevice(pChild);
        }

        // Recurse.
        searchNode(pChild);
    }
}

void entry()
{
    Device *pDev = &Device::root();
    searchNode(pDev);
}

void exit()
{

}

MODULE_NAME("945G");
MODULE_ENTRY(&entry);
MODULE_EXIT(&exit);
MODULE_DEPENDS(0);

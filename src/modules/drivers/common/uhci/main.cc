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

#include <Log.h>
#include <Module.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <machine/Device.h>
#include <usb/UsbConstants.h>
#include "UHCI.h"

void probeDevice(Device *pDev)
{
    NOTICE("UHCI found");

    // Create a new UHCI node
    UHCI *pUHCI = new UHCI(pDev);

    // Replace pDev with pUHCI.
    pUHCI->setParent(pDev->getParent());
    pDev->getParent()->replaceChild(pDev, pUHCI);
}

void entry()
{
    Device::root().searchByClassSubclassAndProgInterface(HCI_CLASS, HCI_SUBCLASS, HCI_PROGIF_UHCI, probeDevice);
}

void exit()
{

}

MODULE_NAME("uhci");
MODULE_ENTRY(&entry);
MODULE_EXIT(&exit);
MODULE_DEPENDS("pci", "usb");

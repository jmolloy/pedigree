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

#include <processor/types.h>
#include <processor/Processor.h>
#include <machine/Device.h>
#include <machine/Machine.h>
#include <utilities/List.h>
#include <Log.h>
#include <Module.h>
#include "Ehci.h"
#include "Ohci.h"
#include "Uhci.h"

enum HcdConstants {
    HCI_CLASS = 0x0C,       // Host Controller PCI class
    HCI_SUBCLASS = 0x03,    // Host Controller PCI subclass
    HCI_PROGIF_UHCI = 0x00, // UHCI PCI programming interface
    HCI_PROGIF_OHCI = 0x10, // OHCI PCI programming interface
    HCI_PROGIF_EHCI = 0x20, // EHCI PCI programming interface
    HCI_PROGIF_XHCI = 0x30, // xHCI PCI programming interface
};

void probeXhci(Device *pDev)
{
    WARNING("USB: xHCI found, not implemented yet!");
    /*
    // Create a new Xhci node
    Xhci *pXhci = new Xhci(pDev);

    // Replace pDev with pXhci, then delete pDev
    pDev->getParent()->replaceChild(pDev, pXhci);
    delete pDev;
    */
}

void probeEhci(Device *pDev)
{
    NOTICE("USB: EHCI found");

    // Create a new Ehci node
    Ehci *pEhci = new Ehci(pDev);

    // Replace pDev with pEhci, then delete pDev
    pDev->getParent()->replaceChild(pDev, pEhci);
    delete pDev;
}

void probeOhci(Device *pDev)
{
    NOTICE("USB: OHCI found");

    // Create a new Ohci node
    Ohci *pOhci = new Ohci(pDev);

    // Replace pDev with pOhci, then delete pDev
    pDev->getParent()->replaceChild(pDev, pOhci);
    delete pDev;
}

#ifdef X86_COMMON
void probeUhci(Device *pDev)
{
    NOTICE("USB: UHCI found");

    // Create a new Uhci node
    Uhci *pUhci = new Uhci(pDev);

    // Replace pDev with pUhci, then delete pDev
    pDev->getParent()->replaceChild(pDev, pUhci);
    delete pDev;
}
#endif

static void entry()
{
    // Interrupts may get disabled on the way here, so make sure they are enabled
    Processor::setInterrupts(true);
    Device::root().searchByClassSubclassAndProgInterface(HCI_CLASS, HCI_SUBCLASS, HCI_PROGIF_XHCI, probeXhci);
    Device::root().searchByClassSubclassAndProgInterface(HCI_CLASS, HCI_SUBCLASS, HCI_PROGIF_EHCI, probeEhci);
    Device::root().searchByClassSubclassAndProgInterface(HCI_CLASS, HCI_SUBCLASS, HCI_PROGIF_OHCI, probeOhci);
#ifdef X86_COMMON
    Device::root().searchByClassSubclassAndProgInterface(HCI_CLASS, HCI_SUBCLASS, HCI_PROGIF_UHCI, probeUhci);
#endif
}

static void exit()
{

}

#ifdef X86_COMMON
MODULE_INFO("usb-hcd", &entry, &exit, "pci", "usb");
#else
#ifdef ARM_COMMON
MODULE_INFO("usb-hcd", &entry, &exit, "usb-glue", "usb");
#else
MODULE_INFO("usb-hcd", &entry, &exit, "usb");
#endif
#endif

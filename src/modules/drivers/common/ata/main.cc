/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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
#include <machine/Disk.h>
#include <machine/Controller.h>
#include "AtaController.h"
#include "IsaAtaController.h"
#include "PciAtaController.h"
#include <Log.h>

static int nController = 0;

void probeIsaDevice(Controller *pDev)
{
  // Create a new AtaController device node.
  IsaAtaController *pController = new IsaAtaController(pDev, nController++);

  // Replace pDev with pController.
  pController->setParent(pDev->getParent());
  pDev->getParent()->replaceChild(pDev, pController);
  
  
  // And delete pDev for good measure.
  //  - Deletion not needed now that AtaController(pDev) destroys pDev. See Device::Device(Device *)
  //delete pDev;
}

void probePiixController(Device *pDev)
{
  static uint8_t interrupt = 14;

  // Create a new AtaController device node.
  Controller *pDevController = new Controller(pDev);
  uintptr_t intnum = pDevController->getInterruptNumber();
  if(intnum == 0)
  {
      // No valid interrupt, handle
      pDevController->setInterruptNumber(interrupt);
      if(interrupt < 15)
          interrupt++;
      else
      {
          ERROR("PCI IDE: Controller found with no IRQ and IRQs 14 and 15 are already allocated");
          delete pDevController;

          return;
      }
  }
  PciAtaController *pController = new PciAtaController(pDevController, nController++);

  // Replace pDev with pController.
  pController->setParent(pDev->getParent());
  pDev->getParent()->replaceChild(pDev, pController);

  // And now we must delete pDev because of the unique way we do this - it's
  // actually totally useless at this stage, and it's been replaced.
  delete pDev;
}

/// Removes the ISA ATA controllers added early in boot
void removeIsaAta(Device *pDev)
{
    for(unsigned int i = 0; i < pDev->getNumChildren(); i++)
    {
        Device *dev = pDev->getChild(i);
        String name;
        dev->getName(name);
        if(dev->getType() == Device::Controller)
        {
            // Get its addresses, and search for "command" and "control".
            bool foundCommand = false;
            bool foundControl = false;
            for (unsigned int j = 0; j < dev->addresses().count(); j++)
            {
                /// \todo Problem with String::operator== - fix.
                if (dev->addresses()[j]->m_Name == "command")
                    foundCommand = true;
                if (dev->addresses()[j]->m_Name == "control")
                    foundControl = true;
            }
            if (foundCommand && foundControl)
            {
                pDev->removeChild(i);
                delete dev;

                // This leaves i one item too far - easily fixed.
                i--;
            }
        }

        // Recurse and keep probing
        removeIsaAta(dev);
    }
}

static void searchNode(Device *pDev, bool bFallBackISA)
{
    // Try for a PIIX IDE controller first. We prefer the PIIX as it enables us
    // to use DMA (and is a little easier to use for device detection).
    static bool bPiixControllerFound = false;
#if 1
    for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
    {
        Device *pChild = pDev->getChild(i);
        // Look for a PIIX controller
        if(pChild->getPciVendorId() == 0x8086)
        {
            // Okay, the vendor is right, but is it the right device?
            /// \todo ICH controller
            if(((pChild->getPciDeviceId() == 0x1230) ||    // PIIX
                (pChild->getPciDeviceId() == 0x7010) ||    // PIIX3
                (pChild->getPciDeviceId() == 0x7111)) &&   // PIIX4
                (pChild->getPciFunctionNumber() == 1))     // IDE Controller
            {
                // Right, we found a PIIX controller. Let's remove the ATA
                // controllers that are created early in the boot (ISA) now
                // so that when we probe the controller we don't run into used
                // ports.
                if(!bPiixControllerFound)
                {
                    removeIsaAta(&Device::root());
                }

                probePiixController(pChild);
                bPiixControllerFound = true;
            }
        }
        // Recurse.
        searchNode(pChild, false);
    }
#endif

    // No PIIX controller found, fall back to ISA
    /// \todo Could also fall back to ICH?
    if(!bPiixControllerFound && bFallBackISA)
    {
        for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
        {
            Device *pChild = pDev->getChild(i);
            // Is this a controller?
            if (pChild->getType() == Device::Controller)
            {
                // Check it's not an ATA controller already.
                String name;
                pChild->getName(name);

                // Get its addresses, and search for "command" and "control".
                bool foundCommand = false;
                bool foundControl = false;
                for (unsigned int j = 0; j < pChild->addresses().count(); j++)
                {
                    /// \todo Problem with String::operator== - fix.
                    if (pChild->addresses()[j]->m_Name == "command")
                        foundCommand = true;
                    if (pChild->addresses()[j]->m_Name == "control")
                        foundControl = true;
                }
                if (foundCommand && foundControl)
                    probeIsaDevice(static_cast<Controller*> (pChild));
            }

            // Recurse.
            searchNode(pChild, true);
        }
    }
}

static void entry()
{
  // Walk the device tree looking for controllers that have 
  // "control" and "command" addresses.
  Device *pDev = &Device::root();  
  searchNode(pDev, true);
}

static void exit()
{

}

MODULE_INFO("ata", &entry, &exit,
#ifdef PPC_COMMON
    "ata-specific"
#elif defined(X86_COMMON)
    "pci"
#else
    0
#endif
    );


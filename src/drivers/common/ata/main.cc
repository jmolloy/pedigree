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
#include <Log.h>

void probeDevice(Controller *pDev)
{
  // Create a new AtaController device node.
  AtaController *pController = new AtaController(pDev);

  // Replace pDev with pController.
  pController->setParent(pDev->getParent());
  pDev->getParent()->replaceChild(pDev, pController);
  
  
  // And delete pDev for good measure.
  //  - Deletion not needed now that AtaController(pDev) destroys pDev. See Device::Device(Device *)
  //delete pDev;
}

void searchNode(Device *pDev)
{
  for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
  {
    Device *pChild = pDev->getChild(i);
    // Is this a controller?
//    if (pChild->getType() == Device::Controller)
//    {
      // Check it's not an ATA controller already.
      String name;
      pChild->getName(name);

      // Get its addresses, and search for "command" and "control".
      bool foundCommand = false;
      bool foundControl = false;
      for (unsigned int j = 0; j < pChild->addresses().count(); j++)
      {
        /// \todo Problem with String::operator== - fix.
        if (!strcmp(pChild->addresses()[j]->m_Name, "command"))
        {
          foundCommand = true;
        }
        if (!strcmp(pChild->addresses()[j]->m_Name, "control"))
          foundControl = true;
      }        
      if (foundCommand && foundControl)
        probeDevice(reinterpret_cast<Controller*> (pChild));
//    }
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
}

void exit()
{

}

const char *g_pModuleName = "ata";
ModuleEntry g_pModuleEntry = &entry;
ModuleExit  g_pModuleExit  = &exit;
#ifdef PPC_COMMON
const char *g_pDepends[] = {"ata-specific", 0};
#else
const char *g_pDepends[] = {0};
#endif

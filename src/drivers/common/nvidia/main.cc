#include <Log.h>
#include <Module.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <utilities/StaticString.h>
#include <utilities/List.h>
#include <machine/Machine.h>
#include <machine/Display.h>
#include <processor/MemoryMappedIo.h>

#include "Dma.h"

Device *searchNode(Device *pDev)
{
  for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
  {
    Device *pChild = pDev->getChild(i);
    String name;
    pChild->getName(name);
    // Get its addresses, and search for fbAddr.
    for (unsigned int j = 0; j < pChild->addresses().count(); j++)
    {
      /// \todo Problem with String::operator== - fix.
      if (pChild->getPciVendorId() == 0x10DE)
      {
        return pChild;
      }
    }

    // Recurse.
    Device *pRet = searchNode(pChild);
    if (pRet) return pRet;
  }
  return 0;
}

void entry()
{
  // Now that we have a framebuffer address, we can (hopefully) find the device in the device tree that owns that address.
  Device *pDevice = searchNode(&Device::root());
  if (!pDevice)
  {
    NOTICE("NVIDIA: No nVidia devices found.");
    return;
  }

  IoBase *pRegs = 0;
  for (int i = 0; i < pDevice->addresses().count(); i++)
  {
    if (!strcmp(pDevice->addresses()[i]->m_Name, "bar0"))
    {
      pRegs = pDevice->addresses()[i]->m_Io;
    }
  }

  IoBase *pFb = 0;
  for (int i = 0; i < pDevice->addresses().count(); i++)
  {
    if (!strcmp(pDevice->addresses()[i]->m_Name, "bar1"))
    {
      pFb = pDevice->addresses()[i]->m_Io;
    }
  }

  // I have an NV34 type, NV30A arch chip.
  /// \todo Fix this driver.
  return;
  uint32_t strapinfo = pRegs->read32(NV32_NV10STRAPINFO);
  NOTICE("Strapinfo: " << Hex << strapinfo << ", ram: " << ((strapinfo&0x3ff00000) >> 20));
// Processor::breakpoint();
  // Pink background to make sure we see any effects - engine may accidentally blit black pixels for example, and we won't pick that
  // up on a black background.
  for (int x = 0; x < 512; x++)
  {
    for (int y = 0; y < 512; y++)
    {
      pFb->write16(0xF00F, (y * 1024 + x) * 2);
    }
  }

  // Draw an image.
  for (int x = 0; x < 128; x++)
  {
    for (int y = 256; y < 128+256; y++)
    {
      pFb->write16(0xFFFF, (y * 1024 + x) * 2);
    }
  }

  // Card detection is done but not coded yet - hardcoded to the device class in my testbed.
  Dma *pDma = new Dma(pRegs, pFb, NV40A, NV40/*NV30A, NV34*/, 0x300000 /* Card RAM - hardcoded for now */);
  pDma->init();
  for(;;);
  pDma->screenToScreenBlit(0, 256, 400, 100, 100, 100);

  pDma->fillRectangle(600, 600, 100, 100);

  for(;;);
}

void exit()
{
}

MODULE_NAME("nvidia");
MODULE_ENTRY(&entry);
MODULE_EXIT(&exit);
MODULE_DEPENDS("TUI", "pci", "vbe");

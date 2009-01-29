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
      if (pChild->getPciVendorId() == 0x10DE || (pChild->getPciVendorId() == 0x15AD/* TMP - VMWare nvidia device */))
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

#define PRAMDAC 0x00680000
#define PCIO 0x0601000
#define PCRTC 0x00600000

void entry()
{
  return;
  // Now that we have a framebuffer address, we can (hopefully) find the device in the device tree that owns that address.
  Device *pDevice = searchNode(&Device::root());
  if (!pDevice)
  {
    WARNING("NVIDIA: No nVidia devices found.");
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

//   pRegs->write32( 0xFFFFFFFF, PRAMDAC+0x324);
//   pRegs->write32( 0xFFFFFFFF, PRAMDAC+0x328);
//   pRegs->write32( (30 | (30 << 16)), PRAMDAC+0x300);
//
//   pRegs->write8(0x31, PCIO+0x3D4);
//   pRegs->write8(0x01, PCIO+0x3D5);
//
  //pRegs->write32(256, PCRTC+0x800);

  // RectName = 0x01020304
  // select area of instance memory: 0x00c04000
  // context: channel 0x00, render object = 1, object type = 0x47 (rectangle)
  // offset in instance memory = 0x4000 / 16 = 0x0400.
  // context = 0x00c70400

  // nhame-context pair goes in the hash table.
  // hash key = 01 xor 02 xor 03 xor 04 xor 00 = 0x04
  //  The 0x00 is the channel id.

// NV driver reckons RAMIN is at 0x00710000

//   pRegs->write32(0x00000001, 0x00710000 + 0x04 * 16);
//   pRegs->write32(0x00c70400, 0x00710000 + 0x04 * 16 + 4);
//
//   // Now fill in the instance ram
//   pRegs->write32(0x00100000, 0x00714000); // 15bpp - DECODE THIS MAGIC
//   pRegs->write32(0x00000000, 0x00714004); // Not used.
//   pRegs->write32(0x00000000, 0x00714008);
//   pRegs->write32(0x00000000, 0x0071400c);
//
//   // Now the FIFO channel is ready for submitting rectangles.
//   // Submit the first rectangle.
//   // Write object's name to channel 0 (subchannel 0).
//   pRegs->write32(0x00000001, 0x00800000);
//   pRegs->write32(0x01000200, 0x00800400);  // x = 0x100, y = 0x200
//   pRegs->write32(0x00100020, 0x00800404); // width = 0x10, height  = 0x20

// I have an NV34 type, NV30A arch chip.
// nvxx_general_powerup -> set_specs
  uint32_t config = pRegs->read32(0x1800);
  NOTICE(config);

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
  Dma *pDma = new Dma(pRegs, pFb, NV30A, NV34, 0x300000 /* Card RAM - hardcoded for now */);
  pDma->init();

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

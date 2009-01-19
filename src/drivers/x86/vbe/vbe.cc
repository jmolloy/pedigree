#include <Log.h>
#include <Module.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <utilities/StaticString.h>
#include <utilities/List.h>
#include <machine/Machine.h>
#include <machine/Display.h>
#include "VbeDisplay.h"

#include <machine/x86_common/Bios.h>

#define REALMODE_PTR(x) ((x[1] << 4) + x[0])

struct vbeControllerInfo {
   char signature[4];             // == "VESA"
   short version;                 // == 0x0300 for VBE 3.0
   short oemString[2];            // isa vbeFarPtr
   unsigned char capabilities[4];
   unsigned short videomodes[2];           // isa vbeFarPtr
   short totalMemory;             // as # of 64KB blocks
} __attribute__((packed));

struct vbeModeInfo {
  short attributes;
  char winA,winB;
  short granularity;
  short winsize;
  short segmentA, segmentB;
  unsigned short realFctPtr[2];
  short pitch; // chars per scanline

  short Xres, Yres;
  char Wchar, Ychar, planes, bpp, banks;
  uint8_t memory_model, bank_size, image_pages;
  char reserved0;

  char red_mask, red_position;
  char green_mask, green_position;
  char blue_mask, blue_position;
  char rsv_mask, rsv_position;
  char directcolor_attributes;

  // --- VBE 2.0 ---
  int framebuffer;
  int offscreen;
  short sz_offscreen; // In KB.
} __attribute__((packed));

Device *searchNode(Device *pDev, uintptr_t fbAddr)
{
  for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
  {
    Device *pChild = pDev->getChild(i);

    // Get its addresses, and search for fbAddr.
    for (unsigned int j = 0; j < pChild->addresses().count(); j++)
    {
      /// \todo Problem with String::operator== - fix.
      if (pChild->addresses()[j]->m_Address == fbAddr)
      {
        return pChild;
      }
    }

    // Recurse.
    return searchNode(pChild, fbAddr);
  }
  return 0;
}

void entry()
{
  Machine::instance().getKeyboard()->setDebugState(false);

  List<Display::ScreenMode*> modeList;

  // Allocate some space for the information structure and prepare for a BIOS call.
  vbeControllerInfo *info = reinterpret_cast<vbeControllerInfo*> (Bios::instance().malloc(/*sizeof(vbeControllerInfo)*/256));
  vbeModeInfo *mode = reinterpret_cast<vbeModeInfo*> (Bios::instance().malloc(/*sizeof(vbeModeInfo)*/256));
  strncpy (info->signature, "VBE2", 4);
  Bios::instance().setAx (0x4F00);
  Bios::instance().setEs (0x0000);
  Bios::instance().setDi (reinterpret_cast<int>(info));

  Bios::instance().executeInterrupt (0x10);

  // Check the signature.
  if (Bios::instance().getAx() != 0x004F || strncmp(info->signature, "VESA", 4) != 0)
  {
    ERROR("VBE: VESA not supported!");
    Bios::instance().free(reinterpret_cast<uintptr_t>(info));
    Bios::instance().free(reinterpret_cast<uintptr_t>(mode));
    return;
  }

  VbeDisplay::VbeVersion vbeVersion;
  switch (info->version)
  {
    case 0x0102:
      vbeVersion = VbeDisplay::Vbe1_2;
      break;
    case 0x0200:
      vbeVersion = VbeDisplay::Vbe2_0;
      break;
    case 0x0300:
      vbeVersion = VbeDisplay::Vbe3_0;
      break;
    default:
      ERROR("VBE: Unrecognised VESA version: " << Hex << info->version);
      Bios::instance().free(reinterpret_cast<uintptr_t>(info));
      Bios::instance().free(reinterpret_cast<uintptr_t>(mode));
      return;
  }

  uint16_t *modes = reinterpret_cast<uint16_t*> (REALMODE_PTR(info->videomodes));
  for (int i = 0; modes[i] != 0xFFFF; i++)
  {
    Bios::instance().setAx(0x4F01);
    Bios::instance().setCx(modes[i]);
    Bios::instance().setEs (0x0000);
    Bios::instance().setDi (reinterpret_cast<int>(mode));

    Bios::instance().executeInterrupt (0x10);

    if (Bios::instance().getAx() != 0x004F) continue;

    // Check if this is a graphics mode with LFB support.
    if ( (mode->attributes & 0x90) != 0x90 ) continue;

    // Check if this is a packed pixel or direct colour mode.
    if (mode->memory_model != 4 && mode->memory_model != 6) continue;

    // Add this pixel mode.
    Display::ScreenMode *pSm = new Display::ScreenMode;
    pSm->id = modes[i];
    pSm->width = mode->Xres;
    pSm->height = mode->Yres;
    pSm->refresh = 0;
    pSm->framebuffer = mode->framebuffer;
    pSm->pf.mRed = mode->red_mask;
    pSm->pf.pRed = mode->red_position;
    pSm->pf.mGreen = mode->green_mask;
    pSm->pf.pGreen = mode->green_position;
    pSm->pf.mBlue = mode->blue_mask;
    pSm->pf.pBlue = mode->blue_position;
    pSm->pf.nBpp = mode->bpp;
    pSm->pf.nPitch = mode->pitch;
    modeList.pushBack(pSm);
  }

  Bios::instance().free(reinterpret_cast<uintptr_t>(info));
  Bios::instance().free(reinterpret_cast<uintptr_t>(mode));

  NOTICE("VESA: Detected compatible display modes:");
  for (List<Display::ScreenMode*>::Iterator it = modeList.begin();
       it != modeList.end();
       it++)
  {
    Display::ScreenMode *pSm = *it;
    NOTICE(Hex << pSm->id << "\t " << Dec << pSm->width << "x" << pSm->height << "x" << pSm->pf.nBpp << "\t " << Hex <<
           pSm->framebuffer);
  }
  NOTICE("VESA: End of compatible display modes.");

  // Now that we have a framebuffer address, we can (hopefully) find the device in the device tree that owns that address.
  //Device *pDevice = searchNode(&Device::root());
}

void exit()
{
}

MODULE_NAME("vbe");
MODULE_ENTRY(&entry);
MODULE_EXIT(&exit);
MODULE_DEPENDS("TUI", "pci");

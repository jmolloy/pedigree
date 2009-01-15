#include <Log.h>
#include <Module.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <utilities/StaticString.h>
#include <machine/Machine.h>

#include <machine/x86_common/Bios.h>

struct vbeControllerInfo {
   char signature[4];             // == "VESA"
   short version;                 // == 0x0300 for VBE 3.0
   short oemString[2];            // isa vbeFarPtr
   unsigned char capabilities[4];
   short videomodes[2];           // isa vbeFarPtr
   short totalMemory;             // as # of 64KB blocks
};

struct vbeModeInfo {
  short attributes;
  char winA,winB;
  short granularity;
  short winsize;
  short segmentA, segmentB;
  int   realFctPtr;
  short pitch; // chars per scanline

  short Xres, Yres;
  char Wchar, Ychar, planes, bpp, banks;
  char memory_model, bank_size, image_pages;
  char reserved0;

  char red_mask, red_position;
  char green_mask, green_position;
  char blue_mask, blue_position;
  char rsv_mask, rsv_position;
  char directcolor_attributes;

  int physbase;  // your LFB address ;)
  int reserved1;
  short reserved2;
};


void entry()
{
  return;
  Machine::instance().getKeyboard()->setDebugState(false);

  // Allocate some space for the information structure and prepare for a BIOS call.
  vbeControllerInfo *info = reinterpret_cast<vbeControllerInfo*> (Bios::instance().malloc(sizeof(vbeControllerInfo)));
  strncpy (info->signature, "VBE2", 4);
  Bios::instance().setAx (0x4F00);
  Bios::instance().setEs (0x0000);
  Bios::instance().setDi (reinterpret_cast<int>(info));

  Bios::instance().executeInterrupt (0x10);

  // Check the signature.
  if (strncmp(info->signature, "VESA", 4) != 0)
  {
    ERROR("VBE: VESA not supported!");
    Bios::instance().free(info);
    return;
  }


}

void exit()
{
}

MODULE_NAME("vbe");
MODULE_ENTRY(&entry);
MODULE_EXIT(&exit);
MODULE_DEPENDS("TUI");

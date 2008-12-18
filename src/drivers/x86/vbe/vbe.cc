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

void entry()
{
  Machine::instance().getKeyboard()->setDebugState(false);

  vbeControllerInfo *info = reinterpret_cast<vbeControllerInfo*> (Bios::instance().malloc(sizeof(vbeControllerInfo)));
  strncpy (info->signature, "VBE2", 4);

  Bios::instance().setAx (0x4F00);
  Bios::instance().setEs (0x0000);
  Bios::instance().setDi (reinterpret_cast<int>(info));
  NOTICE("RAR!!!");
  Bios::instance().executeInterrupt (0x10);

  NOTICE("DI: " << Hex << Bios::instance().getDi());
  NOTICE("ES: " << Hex << Bios::instance().getEs());
  NOTICE("AX: " << Bios::instance().getAx());
  //Processor::breakpoint ();
  //for(;;);
}

void exit()
{
}

MODULE_NAME("vbe");
MODULE_ENTRY(&entry);
MODULE_EXIT(&exit);
MODULE_DEPENDS("TUI");

//include "../../kernel/core/BootIO.h"
//include "../../include/utilities/StaticString.h"
#include "../Module.h"

//extern BootIO bootIO;
extern void hat();

void entry()
{
//  HugeStaticString str;
//  str += "Module is teh loadzor!";
//  bootIO.write(str, BootIO::Blue, BootIO::White);
  hat();
}

void ex()
{
//  HugeStaticString str;
//  str += "Module is teh exit0r!";
//  bootIO.write(str, BootIO::Blue, BootIO::White);
}

const char *g_pModuleName = "test2";
ModuleEntry g_pModuleEntry = &entry;
ModuleExit  g_pModuleExit  = &ex;

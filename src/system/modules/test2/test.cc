#include "../../kernel/core/BootIO.h"
#include "../../include/utilities/StaticString.h"
#include "../Module.h"

extern BootIO bootIO;

void mysym()
{
  HugeStaticString str;
  str += "Module is teh loadzor!";
  bootIO.write(str, BootIO::Blue, BootIO::White);
}

void ex()
{
  HugeStaticString str;
  str += "Module is teh exit0r!";
  bootIO.write(str, BootIO::Blue, BootIO::White);
}

const char *g_pModuleName = "test2";
ModuleEntry g_pModuleEntry = &mysym;
ModuleExit  g_pModuleExit  = &ex;
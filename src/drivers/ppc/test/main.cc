#include "../../../system/kernel/core/BootIO.h"
#include <utilities/StaticString.h>
#include <Log.h>
#include <Module.h>

extern BootIO bootIO;

void entry()
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

const char *g_pModuleName = "test";
ModuleEntry g_pModuleEntry = &entry;
ModuleExit  g_pModuleExit  = &ex;

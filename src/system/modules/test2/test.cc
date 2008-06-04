#include "../../kernel/core/BootIO.h"
#include "../../include/utilities/StaticString.h"

extern BootIO bootIO;

int mysym()
{
  HugeStaticString str;
  str += "Module is teh loadzor!";
  bootIO.write(str, BootIO::Blue, BootIO::White);
}

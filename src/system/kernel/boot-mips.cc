#include "BootstrapInfo.h"
extern "C" void _main(BootstrapStruct_t*);
extern "C" unsigned int __strtab_end;

extern "C" int start()
{
//   _main(0);
  return reinterpret_cast<int>(&__strtab_end);
}

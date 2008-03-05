#include "BootstrapInfo.h"
extern "C" void _main(BootstrapStruct_t*);

extern "C" int start()
{
//   _main(0);
  return 4;
}

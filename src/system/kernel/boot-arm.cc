#include "BootstrapInfo.h"
extern "C" void _main(BootstrapStruct_t*);

extern "C" int start(BootstrapStruct_t *bs)
{
  _main(bs);
  return 0x13371337;
}


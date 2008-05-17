#include "prom.h"

extern "C" void start(unsigned long r3, unsigned long r4, unsigned long r5)
{
  prom_init ( (prom_entry) r5 );
}

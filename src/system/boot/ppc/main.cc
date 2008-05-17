#include "prom.h"

void _start(unsigned long r3, unsigned long r4, unsigned long r5)
{
  prom_init ( (prom_entry) r5 );
}

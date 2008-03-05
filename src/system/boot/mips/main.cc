#include "Elf32.h"

#include "autogen.h"

#define LOAD_ADDR 0x80200000

extern "C" int start()
{

  Elf32 elf("kernel");
  elf.load((uint8_t*)file, 0);
  
  return (int)elf.m_pHeader;
}

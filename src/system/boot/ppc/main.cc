#include "prom.h"
#include "Elf32.h"
#include "autogen.h"
#include "Vga.h"

extern int memset(void *buf, int c, size_t len);
struct BootstrapStruct_t
{
  int (*prom)(struct anon*);
  uint32_t initrd_start;
  uint32_t initrd_end;

  /* ELF information */
  uint32_t num;
  uint32_t size;
  uint32_t addr;
  uint32_t shndx;
};

void writeChar(char c)
{
  prom_putchar(c);
}

void writeStr(const char *str)
{
  char c;
  while ((c = *str++))
    writeChar(c);
}

void writeHex(unsigned int n)
{
    bool noZeroes = true;

    int i;
    unsigned int tmp;
    for (i = 28; i > 0; i -= 4)
    {
        tmp = (n >> i) & 0xF;
        if (tmp == 0 && noZeroes)
        {
            continue;
        }
    
        if (tmp >= 0xA)
        {
            noZeroes = false;
            writeChar (tmp-0xA+'a');
        }
        else
        {
            noZeroes = false;
            writeChar( tmp+'0');
        }
    }
  
    tmp = n & 0xF;
    if (tmp >= 0xA)
    {
        writeChar (tmp-0xA+'a');
    }
    else
    {
        writeChar( tmp+'0');
    }

}
extern void *prom_screen;
extern "C" void _start(unsigned long r3, unsigned long r4, unsigned long r5)
{
  prom_init((prom_entry) r5);

  Elf32 elf("kernel");
  elf.load((uint8_t*)file, 0);
  elf.writeSections();
  int (*main)(struct BootstrapStruct_t*) = (int (*)(struct BootstrapStruct_t*)) elf.getEntryPoint();
  
  struct BootstrapStruct_t bs;

  memset(&bs, 0, sizeof(bs));
  bs.shndx = elf.m_pHeader->shstrndx;
  bs.num = elf.m_pHeader->shnum;
  bs.size = elf.m_pHeader->shentsize;
  bs.addr = (unsigned int)elf.m_pSectionHeaders;
  bs.initrd_start = bs.initrd_end = 0;
  bs.prom = (int (*)(struct anon*)) r5;
  
  // For every section header, set .addr = .offset + m_pBuffer.
  for (int i = 0; i < elf.m_pHeader->shnum; i++)
  {
    elf.m_pSectionHeaders[i].addr = elf.m_pSectionHeaders[i].offset + (uint32_t)elf.m_pBuffer;
  }

  // Cache flush.

  writeStr("About to jump to kernel - entry point 0x");
  writeHex(elf.getEntryPoint());
  writeStr("\n");

  //vga_init();

  main(&bs);
  for(;;);
}

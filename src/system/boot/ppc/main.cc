#include "prom.h"

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

extern "C" void start(unsigned long r3, unsigned long r4, unsigned long r5)
{
  prom_init ( (prom_entry) r5 );
  writeStr("Cunty balls\n");
  writeHex(0xdeadbeef);
}

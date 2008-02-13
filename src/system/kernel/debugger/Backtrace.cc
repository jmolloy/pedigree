/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <Backtrace.h>
#include <utility.h>
#include <Elf32.h>

// TEMP!
extern Elf32 elf;

asm volatile("readEip: pop %eax; jmp *%eax;");
extern "C" unsigned int readEip();

Backtrace::Backtrace()
{
}

Backtrace::~Backtrace()
{
}

void Backtrace::performBacktrace(unsigned int address)
{
  if (address == 0)
  {
    // TODO read EIP using proper machine interface.
    asm volatile("mov %%ebp, %0" : "=r" (address));
  }
  
  unsigned int eip = 0x100000;
  int i = 0;
  
  while (address && eip >= 0x100000 && i < MAX_STACK_FRAMES)
  {
    unsigned int nextAddress    = * ( (unsigned int *) address);
    m_pReturnAddresses[i] = eip = * ((unsigned int *) (address+sizeof(unsigned int)));
    m_pBasePointers[i]          = address;
    address = nextAddress;
    i++;
  }
  
  m_nStackFrames = i;
}

void Backtrace::prettyPrint(char *pBuffer, unsigned int nBufferLength)
{
  // What symbol are we in?
  // TODO grep the memory map for the right ELF to look at.
  
  pBuffer[0] = '\0';
  for (int i = 0; i < m_nStackFrames; i++)
  {
    char pStr[128];
    unsigned int symStart = 0;
    char *pSym = elf.lookupSymbol(m_pReturnAddresses[i], &symStart);
    sprintf(pStr, "[%d] 0x%x <%s>\n", i, m_pReturnAddresses[i], pSym);
    strcat(pBuffer, pStr);
  }
}

unsigned int Backtrace::numStackFrames()
{
  return m_nStackFrames;
}
  
unsigned int Backtrace::getReturnAddress(unsigned int n)
{
//   ASSERT(n < m_nStackFrames);
  return m_pReturnAddresses[n];
}

unsigned int Backtrace::getBasePointer(unsigned int n)
{
//   ASSERT(n < m_nStackFrames);
  return m_pBasePointers[n];
}

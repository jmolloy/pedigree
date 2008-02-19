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
#include <StackFrame.h>

// TEMP!
extern Elf32 elf;

Backtrace::Backtrace()
{
}

Backtrace::~Backtrace()
{
}

void Backtrace::performBacktrace(uintptr_t address)
{
  if (address == 0)
    address = processor::getBasePointer();
  
  int i = 0;
  
  while (address && i < MAX_STACK_FRAMES)
  {
    uintptr_t nextAddress = *reinterpret_cast<uintptr_t *>(address);
    m_pReturnAddresses[i] = *reinterpret_cast<uintptr_t *>(address+sizeof(uintptr_t));
    m_pBasePointers[i] = address;
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
  for (size_t i = 0; i < m_nStackFrames; i++)
  {
    char pStr[128];
    unsigned int symStart = 0;
    const char *pSym = elf.lookupSymbol(m_pReturnAddresses[i], &symStart);
    StackFrame sf(m_pBasePointers[i], pSym);
    sf.prettyPrint(pStr, 128);
//     sprintf(pStr, "[%d] 0x%x <%s>\n", i, m_pReturnAddresses[i], pSym);
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

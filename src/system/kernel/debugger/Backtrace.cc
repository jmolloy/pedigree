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
#include <StaticString.h>

// TEMP!
extern Elf32 elf;

Backtrace::Backtrace()
  : m_nStackFrames(0)
{
}

Backtrace::~Backtrace()
{
}

void Backtrace::performBacktrace(uintptr_t base, uintptr_t instruction)
{
  if (base == 0)
    base = Processor::getBasePointer();
  if (instruction == 0)
    instruction = Processor::getInstructionPointer();
  
  int i = 1;
  m_pBasePointers[0] = base;
  m_pReturnAddresses[0] = instruction;
  
  while (i < MAX_STACK_FRAMES)
  {
    uintptr_t nextAddress = *reinterpret_cast<uintptr_t *>(base);
    if (nextAddress == 0)break;
    m_pReturnAddresses[i] = *reinterpret_cast<uintptr_t *>(base+sizeof(uintptr_t));
    m_pBasePointers[i] = nextAddress;
    base = nextAddress;
    i++;
  }
  
  m_nStackFrames = i;
}

void Backtrace::prettyPrint(HugeStaticString &buf)
{
  // What symbol are we in?
  // TODO grep the memory map for the right ELF to look at.
  for (size_t i = 0; i < m_nStackFrames; i++)
  {
    unsigned int symStart = 0;

    const char *pSym = elf.lookupSymbol(m_pReturnAddresses[i], &symStart);
    LargeStaticString sym(pSym);

    StackFrame sf(m_pBasePointers[i], sym);
    sf.prettyPrint(buf);
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

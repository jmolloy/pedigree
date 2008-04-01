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
#include <utilities/utility.h>
#include <Elf32.h>
#include <StackFrame.h>
#include <utilities/StaticString.h>
#include <Log.h>
#include <DwarfUnwinder.h>

// TEMP!
extern Elf32 elf;
extern uintptr_t start;

Backtrace::Backtrace()
  : m_nStackFrames(0)
{
}

Backtrace::~Backtrace()
{
}

void Backtrace::performBacktrace(InterruptState &state)
{
  // Firstly, can we perform a DWARF backtrace?
  if (elf.debugFrameTable() > 0 /*&& elf.debugFrameTableLength() > 0*/)
  {
    performDwarfBacktrace(state);
  }
  // Can we perform a "normal", base-pointer-linked-list backtrace?
  else if (
#ifdef X86_COMMON
           true
#else
           false
#endif
          )
  {
    performBpBacktrace(state.getBasePointer(), state.getInstructionPointer());
  }
  else
    ERROR ("Backtrace: No backtracing method available!");
}

void Backtrace::performDwarfBacktrace(InterruptState &state)
{
  ProcessorState initial(state);
  ProcessorState next;

  m_pBasePointers[0] = state.getBasePointer();
  m_pReturnAddresses[0] = state.getInstructionPointer();
  
  size_t i = 1;
  DwarfUnwinder du(elf.debugFrameTable(), elf.debugFrameTableLength());
  while (i < MAX_STACK_FRAMES)
  {
    du.unwind(initial, next);
    initial = next; // For next round.
#ifndef MIPS_COMMON
    if (next.getBasePointer() == 0) break;
#else
    uintptr_t symStart;
    elf.lookupSymbol(next.getInstructionPointer(), &symStart);
    if (symStart == reinterpret_cast<uintptr_t> (&start)) break;
#endif
    m_pReturnAddresses[i] = next.getInstructionPointer();
    m_pBasePointers[i] = next.getBasePointer();
    i++;
  }
  
  m_nStackFrames = i;
}

void Backtrace::performBpBacktrace(uintptr_t base, uintptr_t instruction)
{
  if (base == 0)
    base = Processor::getBasePointer();
  if (instruction == 0)
    instruction = Processor::getInstructionPointer();
  
  size_t i = 1;
  m_pBasePointers[0] = base;
  m_pReturnAddresses[0] = instruction;
  
  while (i < MAX_STACK_FRAMES)
  {
    uintptr_t nextAddress = *reinterpret_cast<uintptr_t *>(base);
    if (nextAddress == 0) break;
    m_pReturnAddresses[i] = *reinterpret_cast<uintptr_t *>(base+sizeof(uintptr_t));
    m_pBasePointers[i] = nextAddress;
    base = nextAddress;
    i++;
  }
  
  m_nStackFrames = i;
}

void Backtrace::prettyPrint(HugeStaticString &buf, size_t nFrames)
{
  if (nFrames == 0 || nFrames > m_nStackFrames)
    nFrames = m_nStackFrames;
  // What symbol are we in?
  // TODO grep the memory map for the right ELF to look at.
  for (size_t i = 0; i < nFrames; i++)
  {
    uintptr_t symStart = 0;

    const char *pSym = elf.lookupSymbol(m_pReturnAddresses[i], &symStart);
    LargeStaticString sym(pSym);

    StackFrame sf(m_pBasePointers[i], sym);
    sf.prettyPrint(buf);
  }
}

size_t Backtrace::numStackFrames()
{
  return m_nStackFrames;
}
  
uintptr_t Backtrace::getReturnAddress(size_t n)
{
//   ASSERT(n < m_nStackFrames);
  return m_pReturnAddresses[n];
}

uintptr_t Backtrace::getBasePointer(size_t n)
{
//   ASSERT(n < m_nStackFrames);
  return m_pBasePointers[n];
}

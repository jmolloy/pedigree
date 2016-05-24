/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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
#include <Log.h>
#include <DwarfUnwinder.h>
#include <linker/KernelElf.h>

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
#ifdef DWARF
  // Firstly, can we perform a DWARF backtrace?
  if (KernelElf::instance().debugFrameTable() > 0 /*&& g_pKernel->debugFrameTableLength() > 0*/)
  {
    performDwarfBacktrace(state);
    return;
  }
#else
  WARNING_NOLOCK("Backtracer built without DWARF enabled.");
#endif
  // Can we perform a "normal", base-pointer-linked-list backtrace?
  // Don't lock on the log - we may have hit a backtrace because the Log lock deadlocked
  WARNING_NOLOCK("Dwarf backtracing not available.");
#if defined(X86_COMMON) || defined(HOSTED)
  performBpBacktrace(state.getBasePointer(), state.getInstructionPointer());
#elif defined(ARM_COMMON)
  performArmBacktrace(state.getBasePointer(), state.getInstructionPointer());
#else
  ERROR_NOLOCK("Backtrace: No backtracing method available!");
#endif
}

void Backtrace::performDwarfBacktrace(InterruptState &state)
{
  ProcessorState initial(state);
  ProcessorState next;
  
  size_t i = 0;
  DwarfUnwinder du(KernelElf::instance().debugFrameTable(), KernelElf::instance().debugFrameTableLength());
  uintptr_t frameBase;
  while (i < MAX_STACK_FRAMES)
  {
    if (!du.unwind(initial, next, frameBase))
    {
      m_pReturnAddresses[i++] = initial.getInstructionPointer();
      break;
    }
    
    m_pStates[i] = next;
    m_pBasePointers[i] = frameBase;
    // This may look strange but it's correct - the i'th instruction pointer is the (i-1)'th return
    // address.
    m_pReturnAddresses[i] = initial.getInstructionPointer();
    initial = next;
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
        // Sanity check: would we fault by reading this address?
        if (Processor::information().getVirtualAddressSpace().isMapped(reinterpret_cast<void*>(base)) &&
            Processor::information().getVirtualAddressSpace().isMapped(reinterpret_cast<void*>(base+sizeof(uintptr_t))))
        {
            uintptr_t nextAddress = *reinterpret_cast<uintptr_t *>(base);

            m_pReturnAddresses[i] = *reinterpret_cast<uintptr_t *>(base + sizeof(uintptr_t));
            m_pBasePointers[i] = nextAddress;

            base = nextAddress;

            m_pStates[i].setBasePointer(m_pBasePointers[i]);
            m_pStates[i].setInstructionPointer(m_pReturnAddresses[i]);

            i++;

            if (nextAddress == 0) break;
        }
        else
        {
          break;
        }
    }
  
    m_nStackFrames = i;
}

#ifdef ARM_COMMON
void Backtrace::performArmBacktrace(uintptr_t base, uintptr_t instruction)
{
    if (base == 0)
        base = Processor::getBasePointer();
    if (instruction == 0)
        instruction = Processor::getInstructionPointer();

    size_t i = 1;
    m_pBasePointers[0] = base;
    m_pReturnAddresses[0] = instruction;

    VirtualAddressSpace &va = Processor::information().getVirtualAddressSpace();

    while (i < MAX_STACK_FRAMES)
    {
        uintptr_t *nextFramePointer = reinterpret_cast<uintptr_t *>(base - (3 * sizeof(uintptr_t)));
        uintptr_t *returnAddressPointer = reinterpret_cast<uintptr_t *>(base - sizeof(uintptr_t));

        // Sanity check: would we fault by reading this address?
        if (va.isMapped(nextFramePointer) && (i && va.isMapped(returnAddressPointer)))
        {
            // EABI frame pointer layout:
            // FP[-1] = lr (return address)
            // FP[-3] = previous frame (fp)

            uintptr_t nextFrame = *nextFramePointer;
            m_pReturnAddresses[i] = *returnAddressPointer;
            m_pBasePointers[i] = nextFrame;
            base = nextFrame;

            m_pStates[i].setBasePointer(m_pBasePointers[i]);
            m_pStates[i].setInstructionPointer(m_pReturnAddresses[i]);

            i++;

            if (nextFrame == 0)
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    m_nStackFrames = i;
}
#endif

void Backtrace::prettyPrint(HugeStaticString &buf, size_t nFrames, size_t nFromFrame)
{
  if (nFrames == 0 || nFrames > m_nStackFrames)
    nFrames = m_nStackFrames;
  // What symbol are we in?
  // TODO grep the memory map for the right ELF to look at.
  
  for (size_t i = nFromFrame; i < nFrames+nFromFrame; i++)
  {
    uintptr_t symStart = 0;

    const char *pSym = KernelElf::instance().globalLookupSymbol(m_pReturnAddresses[i], &symStart);
    if (pSym == 0)
    {
      buf.append(m_pReturnAddresses[i], 16);
      buf += "\n";
    }
    else
    {
      LargeStaticString sym(pSym);

      buf += "[";
      buf.append(m_pReturnAddresses[i], 16);
      buf += "] ";
      StackFrame sf(m_pStates[i], m_pBasePointers[i], sym);
      
      sf.prettyPrint(buf);
    }
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

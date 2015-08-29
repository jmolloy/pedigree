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

#include <processor/Processor.h>
#include "PhysicalMemoryManager.h"
#include <process/initialiseMultitasking.h>
#include <process/Thread.h>

using namespace __pedigree_hosted;

#include <signal.h>

bool Processor::m_bInterrupts;

typedef void (*jump_func_t)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);

void Processor::initialisationDone()
{
  HostedPhysicalMemoryManager::instance().initialisationDone();
}

void Processor::initialise1(const BootstrapStruct_t &Info)
{
  HostedPhysicalMemoryManager &physicalMemoryManager = HostedPhysicalMemoryManager::instance();
  physicalMemoryManager.initialise(Info);
  setInterrupts(false);
  m_Initialised = 1;
}

void Processor::initialise2(const BootstrapStruct_t &Info)
{
  initialiseMultitasking();
  m_Initialised = 2;
}

void Processor::identify(HugeStaticString &str)
{
  str.clear();
  str.append("Hosted Processor");
}

uintptr_t Processor::getInstructionPointer()
{
  return reinterpret_cast<uintptr_t>(__builtin_return_address(0));
}

uintptr_t Processor::getStackPointer()
{
  return 0;
}

uintptr_t Processor::getBasePointer()
{
  return reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
}

bool Processor::saveState(SchedulerState &state)
{
  return setjmp(state.state) == 1;
}

void Processor::restoreState(SchedulerState &state, volatile uintptr_t *pLock)
{
  longjmp(state.state, 0);
}

void Processor::restoreState(SyscallState &state, volatile uintptr_t *pLock)
{
  /// \todo implement (green thread style context switch)
}

void Processor::jumpKernel(volatile uintptr_t *pLock, uintptr_t address,
  uintptr_t stack, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4)
{
    if(pLock)
        *pLock = 1;

    register uintptr_t p1_r = p1;
    register uintptr_t p2_r = p2;
    register uintptr_t p3_r = p3;
    register uintptr_t p4_r = p4;
    register jump_func_t target = reinterpret_cast<jump_func_t>(address);

    if(stack)
    {
      asm volatile("mov %0, %%rsp" :: "r" (stack));
    }
    asm volatile("" ::: "memory");

    target(p1_r, p2_r, p3_r, p4_r);
    Thread::threadExited();
}

void Processor::jumpUser(volatile uintptr_t *pLock, uintptr_t address,
  uintptr_t stack, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4)
{
}

void Processor::switchAddressSpace(VirtualAddressSpace &AddressSpace)
{
}

void Processor::setTlsBase(uintptr_t newBase)
{
}

size_t Processor::getDebugBreakpointCount()
{
  return 0;
}

uintptr_t Processor::getDebugBreakpoint(size_t nBpNumber,
                                        DebugFlags::FaultType &nFaultType,
                                        size_t &nLength,
                                        bool &bEnabled)
{
  // no-op on hosted
  return 0;
}

void Processor::enableDebugBreakpoint(size_t nBpNumber,
                                      uintptr_t nLinearAddress,
                                      DebugFlags::FaultType nFaultType,
                                      size_t nLength)
{
  // no-op on hosted
}

void Processor::disableDebugBreakpoint(size_t nBpNumber)
{
  // no-op on hosted
}

void Processor::setInterrupts(bool bEnable)
{
  // Block signals to toggle "interrupts".
  int how = SIG_BLOCK;
  if(bEnable)
    how = SIG_UNBLOCK;

  sigset_t set;
  sigfillset(&set);

  sigprocmask(how, &set, 0);

  m_bInterrupts = bEnable;
}

bool Processor::getInterrupts()
{
  return m_bInterrupts;
}

void Processor::setSingleStep(bool bEnable, InterruptState &state)
{
  // no-op on hosted
}

void Processor::invalidate(void *pAddress)
{
  // no-op on hosted
}

namespace __processor_cc_hosted {
    #include <unistd.h>
    #include <stdlib.h>
}

void Processor::_reset()
{
    // Just exit.
    __processor_cc_hosted::exit(0);
}

void Processor::_haltUntilInterrupt()
{
    __processor_cc_hosted::pause();
}

/// \todo not here
void PerProcessorScheduler::deleteThreadThenRestoreState(Thread *pThread, SchedulerState &newState)
{
}

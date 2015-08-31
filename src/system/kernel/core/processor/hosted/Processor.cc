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
#include <processor/PageFaultHandler.h>
#include "PhysicalMemoryManager.h"
#include "InterruptManager.h"
#include "SyscallManager.h"
#include "VirtualAddressSpace.h"
#include <process/initialiseMultitasking.h>
#include <process/Thread.h>

namespace __pedigree_hosted {};
using namespace __pedigree_hosted;

#include <signal.h>
#include <setjmp.h>

bool Processor::m_bInterrupts;

typedef void (*jump_func_t)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);

void Processor::initialisationDone()
{
  HostedPhysicalMemoryManager::instance().initialisationDone();
}

void Processor::initialise1(const BootstrapStruct_t &Info)
{
  HostedInterruptManager::initialiseProcessor();
  PageFaultHandler::instance().initialise();
  HostedPhysicalMemoryManager::instance().initialise(Info);
  HostedSyscallManager::initialiseProcessor();
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
  jmp_buf _state;
  if(sigsetjmp(_state, 0) == 1)
    return true;

  // Handle opaque types.
  memcpy(state.state, _state, sizeof(jmp_buf));
  return false;
}

void Processor::restoreState(SchedulerState &state, volatile uintptr_t *pLock)
{
  jmp_buf _state;
  if(pLock)
      *pLock = 1;
  memcpy(_state, state.state, sizeof(jmp_buf));
  siglongjmp(_state, 0);
  // Does not return.
}

void Processor::restoreState(SyscallState &state, volatile uintptr_t *pLock)
{
  /// \todo implement (green thread style context switch)
  panic("Processor::restoreState(SyscallState&) NOT implemented.");
}

void Processor::jumpUser(volatile uintptr_t *pLock, uintptr_t address,
  uintptr_t stack, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4)
{
  // Same thing as jumping to kernel space.
  jumpKernel(pLock, address, stack, p1, p2, p3, p4);
}

void Processor::switchAddressSpace(VirtualAddressSpace &AddressSpace)
{
  ProcessorInformation &info = Processor::information();
  if(&info.getVirtualAddressSpace() != &AddressSpace)
  {
      HostedVirtualAddressSpace::switchAddressSpace(
        info.getVirtualAddressSpace(), AddressSpace);
  }
}

void Processor::setTlsBase(uintptr_t newBase)
{
  ERROR("Processor::setTlsBase unimplemented");
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
  sigset_t set;
  if(bEnable)
    sigemptyset(&set);
  else
  {
    sigemptyset(&set);

    // Only SIGUSR1 and SIGUSR2 are true "interrupts". The rest are all
    // more like exceptions, which we are okay with triggering even if bEnable
    // is false.
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
  }

  sigprocmask(SIG_SETMASK, &set, 0);

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
    #include <signal.h>
    #include <sched.h>
}

using namespace __processor_cc_hosted;

void Processor::_breakpoint()
{
    sigset_t set;
    sigset_t oset;
    sigemptyset(&set);
    sigemptyset(&oset);
    sigaddset(&set, SIGTRAP);
    sigprocmask(SIG_UNBLOCK, &set, &oset);
    raise(SIGTRAP);
    sigprocmask(SIG_SETMASK, &oset, 0);
}

void Processor::_reset()
{
    // Just exit.
    exit(0);
}

void Processor::_haltUntilInterrupt()
{
    sigset_t set;
    sigemptyset(&set);
    sigsuspend(&set);
}

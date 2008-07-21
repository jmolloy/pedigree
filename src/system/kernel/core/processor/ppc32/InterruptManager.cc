/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

#include "InterruptManager.h"
#include <machine/Machine.h>
#include <machine/types.h>
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <Debugger.h>
#include <Log.h>
#include <panic.h>
#include <machine/openfirmware/OpenFirmware.h>
#include <machine/openfirmware/Device.h>
#include <LockGuard.h>

extern "C" int isr_reset;
extern "C" int isr_machine_check;
extern "C" int isr_dsi;
extern "C" int isr_isi;
extern "C" int isr_interrupt;
extern "C" int isr_alignment;
extern "C" int isr_program;
extern "C" int isr_fpu;
extern "C" int isr_decrementer;
extern "C" int isr_sc;
extern "C" int isr_trace;
extern "C" int isr_perf_mon;
extern "C" int isr_instr_breakpoint;
extern "C" int isr_system_management;
extern "C" int isr_thermal_management;

const char *g_pExceptions[] = {
  "System reset",
  "Machine check",
  "DSI",
  "ISI",
  "External interrupt",
  "Alignment",
  "Program",
  "Floating-point unavailable",
  "Decrementer",
  "System call",
  "Trace",
  "Performance monitor",
  "Instruction address breakpoint",
  "System management interrupt",
  "Thermal management interrupt"
};

PPC32InterruptManager PPC32InterruptManager::m_Instance;

SyscallManager &SyscallManager::instance()
{
  return PPC32InterruptManager::instance();
}
InterruptManager &InterruptManager::instance()
{
  return PPC32InterruptManager::instance();
}

bool PPC32InterruptManager::registerInterruptHandler(size_t interruptNumber, InterruptHandler *pHandler)
{
  LockGuard<Spinlock> lockGuard(m_Lock);

  if (UNLIKELY(interruptNumber >= 256 || interruptNumber == SYSCALL_INTERRUPT_NUMBER))
    return false;
  if (UNLIKELY(pHandler != 0 && m_pHandler[interruptNumber] != 0))
    return false;
  if (UNLIKELY(pHandler == 0 && m_pHandler[interruptNumber] == 0))
    return false;

  m_pHandler[interruptNumber] = pHandler;
  return true;
}

#ifdef DEBUGGER
  bool PPC32InterruptManager::registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *pHandler)
  {
    LockGuard<Spinlock> lockGuard(m_Lock);

    if (UNLIKELY(interruptNumber >= 256 || interruptNumber == SYSCALL_INTERRUPT_NUMBER))
      return false;
    if (UNLIKELY(pHandler != 0 && m_pDbgHandler[interruptNumber] != 0))
      return false;
    if (UNLIKELY(pHandler == 0 && m_pDbgHandler[interruptNumber] == 0))
      return false;

    m_pDbgHandler[interruptNumber] = pHandler;
    return true;
  }
  size_t PPC32InterruptManager::getBreakpointInterruptNumber()
  {
    return TRAP_INTERRUPT_NUMBER;
  }
  size_t PPC32InterruptManager::getDebugInterruptNumber()
  {
    return TRACE_INTERRUPT_NUMBER;
  }
#endif

bool PPC32InterruptManager::registerSyscallHandler(Service_t Service, SyscallHandler *handler)
{
  return true;
}

void PPC32InterruptManager::initialiseProcessor()
{
  // We know that we get called before the virtual address space is initialised, so
  // we'll have to do the identity mapping ourselves. How crude!
  OFDevice chosen (OpenFirmware::instance().findDevice("/chosen"));
  OFDevice mmu (chosen.getProperty("mmu"));

  // Identity map the lower area of memory.
  mmu.executeMethod("map", 4,
                    reinterpret_cast<OFParam>(-1),
                    reinterpret_cast<OFParam>(0x3000),
                    reinterpret_cast<OFParam>(0x0),
                    reinterpret_cast<OFParam>(0x0));
  // Copy the interrupt handlers into lower memory.
  memcpy(reinterpret_cast<void*> (0x0100), &isr_reset, 0x100);
  memcpy(reinterpret_cast<void*> (0x0200), &isr_machine_check, 0x100);
  memcpy(reinterpret_cast<void*> (0x0300), &isr_dsi, 0x100);
  memcpy(reinterpret_cast<void*> (0x0400), &isr_isi, 0x100);
  memcpy(reinterpret_cast<void*> (0x0500), &isr_interrupt, 0x100);
  memcpy(reinterpret_cast<void*> (0x0600), &isr_alignment, 0x100);
  memcpy(reinterpret_cast<void*> (0x0700), &isr_program, 0x100);
  memcpy(reinterpret_cast<void*> (0x0800), &isr_fpu, 0x100);
  memcpy(reinterpret_cast<void*> (0x0900), &isr_decrementer, 0x100);
  memcpy(reinterpret_cast<void*> (0x0C00), &isr_sc, 0x100);
  memcpy(reinterpret_cast<void*> (0x0D00), &isr_trace, 0x100);
  memcpy(reinterpret_cast<void*> (0x0F00), &isr_perf_mon, 0x100);
  memcpy(reinterpret_cast<void*> (0x1300), &isr_instr_breakpoint, 0x100);
  memcpy(reinterpret_cast<void*> (0x1400), &isr_system_management, 0x100);
  memcpy(reinterpret_cast<void*> (0x1700), &isr_thermal_management, 0x100);

  for (uintptr_t i = 0x0; i < 0x1800; i += 4)
    Processor::flushDCache(i);

  asm volatile("sync");

  for (uintptr_t i = 0; i < 0x1800; i += 4)
    Processor::invalidateICache(i);
  
  asm volatile("sync");
  asm volatile("isync");
}

void PPC32InterruptManager::interrupt(InterruptState &interruptState)
{
  // TODO: Needs locking
  size_t intNumber = interruptState.getInterruptNumber();

  #ifdef DEBUGGER
    // Call the kernel debugger's handler, if any
    if (m_Instance.m_pDbgHandler[intNumber] != 0)
      m_Instance.m_pDbgHandler[intNumber]->interrupt(intNumber, interruptState);
  #endif

  // Call the syscall handler, if it is the syscall interrupt
  if (intNumber == SYSCALL_INTERRUPT_NUMBER)
  {
    size_t serviceNumber = interruptState.getSyscallService();
    if (LIKELY(serviceNumber < serviceEnd && m_Instance.m_pSyscallHandler[serviceNumber] != 0))
    m_Instance.m_pSyscallHandler[serviceNumber]->syscall(interruptState);
  }
  else if (m_Instance.m_pHandler[intNumber] != 0)
    m_Instance.m_pHandler[intNumber]->interrupt(intNumber, interruptState);
  else if (intNumber != 6 && intNumber != 10)
  {
    // TODO:: Check for debugger initialisation.
    static LargeStaticString e;
    e.clear();
    e.append ("Exception #");
    e.append (interruptState.m_IntNumber, 10);
    e.append (": \"");
    e.append (g_pExceptions[intNumber]);
    e.append ("\"");
#ifdef DEBUGGER
    Debugger::instance().start(interruptState, e);
#else
    FATAL("SRR0: " << Hex << interruptState.m_Srr0 << ", SRR1: " << interruptState.m_Srr1);
    FATAL("DAR: " << interruptState.m_Dar << ", DSISR: " << interruptState.m_Dsisr);
    panic(e);
#endif
  }

  // Some interrupts (like Program) require the PC to be advanced before returning.
  if (intNumber == TRAP_INTERRUPT_NUMBER)
  {
    interruptState.m_Srr0 += 4;
  }
}

PPC32InterruptManager::PPC32InterruptManager() :
  m_Lock()
{
}
PPC32InterruptManager::~PPC32InterruptManager()
{
}

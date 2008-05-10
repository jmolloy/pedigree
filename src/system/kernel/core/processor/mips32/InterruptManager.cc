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
#include <processor/TlbManager.h>
#include <machine/Machine.h>
#include <machine/types.h>
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <Debugger.h>
#include <Log.h>

#define SYSCALL_INTERRUPT_NUMBER 8
#define BREAKPOINT_INTERRUPT_NUMBER 9

const char *g_ExceptionNames[32] = {
  "Interrupt",
  "TLB modification exception",
  "TLB exception (load or instruction fetch)",
  "TLB exception (store)",
  "Address error exception (load or instruction fetch)",
  "Address error exception (store)",
  "Bus error exception (instruction fetch)",
  "Bus error exception (data: load or store)",
  "Syscall exception",
  "Breakpoint exception",
  "Reserved instruction exception",
  "Coprocessor unusable exception",
  "Arithmetic overflow exception",
  "Trap exception",
  "LDCz/SDCz to uncached address",
  "Virtual coherency exception",
  "Machine check exception",
  "Floating point exception",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Watchpoint exception",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
};

MIPS32InterruptManager MIPS32InterruptManager::m_Instance;

SyscallManager &SyscallManager::instance()
{
  return MIPS32InterruptManager::instance();
}
InterruptManager &InterruptManager::instance()
{
  return MIPS32InterruptManager::instance();
}

bool MIPS32InterruptManager::registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler)
{
  // TODO: Needs locking
  if (UNLIKELY(interruptNumber >= 256))
    return false;
  if (UNLIKELY(handler != 0 && m_Handler[interruptNumber] != 0))
    return false;
  if (UNLIKELY(handler == 0 && m_Handler[interruptNumber] == 0))
    return false;

  m_Handler[interruptNumber] = handler;
  return true;
}

bool MIPS32InterruptManager::registerExternalInterruptHandler(size_t interruptNumber, InterruptHandler *handler)
{
  // TODO: Needs locking
  if (UNLIKELY(interruptNumber >= 8))
    return false;
  if (UNLIKELY(handler != 0 && m_ExternalHandler[interruptNumber] != 0))
    return false;
  if (UNLIKELY(handler == 0 && m_ExternalHandler[interruptNumber] == 0))
    return false;

  m_ExternalHandler[interruptNumber] = handler;
  return true;
}

#ifdef DEBUGGER

  bool MIPS32InterruptManager::registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler)
  {
    // TODO: Needs locking
    if (UNLIKELY(interruptNumber >= 256))
      return false;
    if (UNLIKELY(handler != 0 && m_DbgHandler[interruptNumber] != 0))
      return false;
    if (UNLIKELY(handler == 0 && m_DbgHandler[interruptNumber] == 0))
      return false;

    m_DbgHandler[interruptNumber] = handler;
    return true;
  }
  size_t MIPS32InterruptManager::getBreakpointInterruptNumber()
  {
    return 3;
  }
  size_t MIPS32InterruptManager::getDebugInterruptNumber()
  {
    return 1;
  }

#endif

bool MIPS32InterruptManager::registerSyscallHandler(Service_t Service, SyscallHandler *handler)
{
  //TODO: Needs locking

  if (UNLIKELY(Service >= serviceEnd))
    return false;
  if (UNLIKELY(handler != 0 && m_SyscallHandler[Service] != 0))
    return false;
  if (UNLIKELY(handler == 0 && m_SyscallHandler[Service] == 0))
    return false;

  m_SyscallHandler[Service] = handler;
  return true;
}
extern "C" void mips32_exception(void);
void MIPS32InterruptManager::initialiseProcessor()
{
  // Exception handler goes at 0x8000 0180
  // Here we generate some exception handling code.
  uint32_t pCode[4];
  
  // lui $k0, <upper 16 bits of exception handler>
  pCode[0] = 0x3c1a0000 | (reinterpret_cast<uint32_t> (&mips32_exception) >> 16);
  // ori $k0, $k0, <lower 16 bits of exception handler>
  pCode[1] = 0x375a0000 | (reinterpret_cast<uint32_t> (&mips32_exception) & 0x0000FFFF);
  // jr $k0
  pCode[2] = 0x03400008;
  // nop (delay slot)
  pCode[3] = 0x00000000;
  
  // Now poke that exception handling stub into memory.
  memcpy(reinterpret_cast<void*>(KSEG1(0x80)),
         reinterpret_cast<void*>(pCode),
         32*4);
  memcpy(reinterpret_cast<void*>(KSEG1(0x100)),
         reinterpret_cast<void*>(pCode),
         32*4);
  memcpy(reinterpret_cast<void*>(KSEG1(0x180)),
         reinterpret_cast<void*>(pCode),
         32*4);
  memcpy(reinterpret_cast<void*>(KSEG1(0x200)),
         reinterpret_cast<void*>(pCode),
         32*4);

  // Do the same for the TLB refill handler.

  // lui $k0, <upper 16 bits of exception handler>
  pCode[0] = 0x3c1a0000 | (reinterpret_cast<uint32_t> (&TlbManager::interruptAsm) >> 16);
  // ori $k0, $k0, <lower 16 bits of exception handler>
  pCode[1] = 0x375a0000 | (reinterpret_cast<uint32_t> (&TlbManager::interruptAsm) & 0x0000FFFF);
  // jr $k0
  pCode[2] = 0x03400008;
  // nop (delay slot)
  pCode[3] = 0x00000000;

  memcpy(reinterpret_cast<void*>(KSEG1(0x0)),
         reinterpret_cast<void*>(pCode),
         32*4);

  
  // Invalidate the instruction cache - force a reload of the exception handlers.
  for (uintptr_t i = KSEG0(0); i < KSEG0(0x200); i += 0x80)
    Processor::invalidateICache(i);
}

void MIPS32InterruptManager::interrupt(InterruptState &interruptState)
{
  // We don't want interrupts, but we don't need to be in the exception level.
  uintptr_t sr = interruptState.m_Sr;
  sr &= ~SR_IE;  // Disable interrupts.
  sr &= ~SR_EXL; // Remove us from being in exception privilege level.
  // TODO set SE_KSU.
  asm volatile ("mtc0 %0, $12" :: "r" (sr));

  // Read the cause register.
  uint32_t cause;
  asm volatile("mtc0 %0, $13" : "=r" (cause));
  
  // Mask off interrupts not enabled.
  cause &= interruptState.m_Sr;

  // Find the external interrupts pending - TODO mask off masked interrupts.
  cause = (cause >> 8) & 0xFF;

  // Find the lowest numbered interrupt pending - we will handle this one.
  uint32_t externalInt = 0xFF;
  for (int i = 0; i < 8; i++)
  {
    if (cause & (1<<i))
    {
      externalInt = i;
      break;
    }
  }

  // TODO: Needs locking
  size_t intNumber = interruptState.getInterruptNumber();

  #ifdef DEBUGGER
    // Call the kernel debugger's handler, if any
    if (m_Instance.m_DbgHandler[intNumber] != 0)
      m_Instance.m_DbgHandler[intNumber]->interrupt(intNumber, interruptState);
  #endif

  // Call the syscall handler, if it is the syscall interrupt
  if (intNumber == SYSCALL_INTERRUPT_NUMBER)
  {
    size_t serviceNumber = interruptState.getSyscallService();
    if (LIKELY(serviceNumber < serviceEnd && m_Instance.m_SyscallHandler[serviceNumber] != 0))
      m_Instance.m_SyscallHandler[serviceNumber]->syscall(interruptState);
  }
  // External interrupt, handled?
  else if (intNumber == 0 && m_Instance.m_ExternalHandler[externalInt])
    m_Instance.m_ExternalHandler[externalInt]->interrupt(externalInt, interruptState);
  // Call the normal interrupt handler, if any, otherwise
  else if (m_Instance.m_Handler[intNumber] != 0)
    m_Instance.m_Handler[intNumber]->interrupt(intNumber, interruptState);
  else
  {
    // TODO:: Check for debugger initialisation.
    static LargeStaticString e;
    e.clear();
    e.append ("Exception #");
    e.append (intNumber, 10);
    e.append (": \"");
    e.append (g_ExceptionNames[intNumber]);
    e.append ("\"");
    Debugger::instance().start(interruptState, e);
  }

  // If this was a trap or breakpoint instruction, we need to increase the program counter a bit.
  if (intNumber == 9 || intNumber == 13)
  {
    // ...Unless we were in a branch delay slot!
    if (!interruptState.branchDelay())
    {
      interruptState.m_Epc += 4;
    }
  }
}

MIPS32InterruptManager::MIPS32InterruptManager()
{
  // Initialise the pointers to the interrupt handler
  for (size_t i = 0;i < 64;i++)
  {
    m_Handler[i] = 0;
    if (i < 8)
      m_ExternalHandler[i] = 0;
    #ifdef DEBUGGER
      m_DbgHandler[i] = 0;
    #endif
  }

  // Initialise the pointers to the syscall handler
  for (size_t i = 0;i < serviceEnd;i++)
    m_SyscallHandler[i] = 0;
}
MIPS32InterruptManager::~MIPS32InterruptManager()
{
}

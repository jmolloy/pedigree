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
#include "InterruptManager.h"
#include <machine/Machine.h>
#include <machine/types.h>
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <panic.h>
#ifdef DEBUGGER
  #include <Debugger.h>
#endif

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

ARM926EInterruptManager ARM926EInterruptManager::m_Instance;

SyscallManager &SyscallManager::instance()
{
  return ARM926EInterruptManager::instance();
}
InterruptManager &InterruptManager::instance()
{
  return ARM926EInterruptManager::instance();
}

bool ARM926EInterruptManager::registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler)
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

#ifdef DEBUGGER

  bool ARM926EInterruptManager::registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler)
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
  size_t ARM926EInterruptManager::getBreakpointInterruptNumber()
  {
    return 3;
  }
  size_t ARM926EInterruptManager::getDebugInterruptNumber()
  {
    return 1;
  }

#endif

bool ARM926EInterruptManager::registerSyscallHandler(Service_t Service, SyscallHandler *handler)
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
//extern "C" void mips32_exception(void);
void ARM926EInterruptManager::initialiseProcessor()
{
  // TODO: implement
}

void ARM926EInterruptManager::interrupt(InterruptState &interruptState)
{
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
#ifdef DEBUGGER
    Debugger::instance().start(interruptState, e);
#else
    panic(e);
#endif
  }

  // If this was a trap or breakpoint instruction, we need to increase the program counter a bit.
//   if (intNumber == 9 || intNumber == 13)
//   {
    // ...Unless we were in a branch delay slot!
    /*if (!interruptState.branchDelay())
    {
      interruptState.m_Pc += 4;
    }*/
//   }
}

ARM926EInterruptManager::ARM926EInterruptManager()
{
  // Initialise the pointers to the interrupt handler
  for (size_t i = 0;i < 256;i++)
  {
    m_Handler[i] = 0;
    #ifdef DEBUGGER
      m_DbgHandler[i] = 0;
    #endif
  }

  // Initialise the pointers to the syscall handler
  for (size_t i = 0;i < serviceEnd;i++)
    m_SyscallHandler[i] = 0;
}
ARM926EInterruptManager::~ARM926EInterruptManager()
{
}

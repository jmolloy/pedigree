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

void MIPS32InterruptManager::initialiseProcessor()
{

}

void MIPS32InterruptManager::interrupt(InterruptState &interruptState)
{
  // TODO: Needs locking

//   size_t intNumber = interruptState.getInterruptNumber();
// 
//   #ifdef DEBUGGER
//     // Call the kernel debugger's handler, if any
//     if (m_Instance.m_DbgHandler[intNumber] != 0)
//       m_Instance.m_DbgHandler[intNumber]->interrupt(intNumber, interruptState);
//   #endif
// 
//   // Call the syscall handler, if it is the syscall interrupt
//   if (intNumber == SYSCALL_INTERRUPT_NUMBER)
//   {
//     size_t serviceNumber = interruptState.getSyscallService();
//     if (LIKELY(serviceNumber < serviceEnd && m_Instance.m_SyscallHandler[serviceNumber] != 0))
//       m_Instance.m_SyscallHandler[serviceNumber]->syscall(interruptState);
//   }
//   // Call the normal interrupt handler, if any, otherwise
//   else if (m_Instance.m_Handler[intNumber] != 0)
//     m_Instance.m_Handler[intNumber]->interrupt(intNumber, interruptState);
}

MIPS32InterruptManager::MIPS32InterruptManager()
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
MIPS32InterruptManager::~MIPS32InterruptManager()
{
}

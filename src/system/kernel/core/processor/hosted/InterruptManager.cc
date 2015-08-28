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

#include <panic.h>
#include <LockGuard.h>
#include <utilities/StaticString.h>
#include "InterruptManager.h"
#if defined(DEBUGGER)
  #include <Debugger.h>
#endif

#ifdef THREADS
#include <process/Process.h>
#include <Subsystem.h>
#endif

HostedInterruptManager HostedInterruptManager::m_Instance;

InterruptManager &InterruptManager::instance()
{
  return HostedInterruptManager::instance();
}

bool HostedInterruptManager::registerInterruptHandler(size_t nInterruptNumber,
                                                   InterruptHandler *pHandler)
{
  // Lock the class until the end of the function
  LockGuard<Spinlock> lock(m_Lock);

  // Sanity checks
  if (UNLIKELY(nInterruptNumber >= 256))
    return false;
  if (UNLIKELY(pHandler != 0 && m_pHandler[nInterruptNumber] != 0))
    return false;
  if (UNLIKELY(pHandler == 0 && m_pHandler[nInterruptNumber] == 0))
    return false;

  // Change the pHandler
  m_pHandler[nInterruptNumber] = pHandler;

  return true;
}

#if defined(DEBUGGER)

  bool HostedInterruptManager::registerInterruptHandlerDebugger(size_t nInterruptNumber,
                                                             InterruptHandler *pHandler)
  {
    // Lock the class until the end of the function
    LockGuard<Spinlock> lock(m_Lock);

    // Sanity checks
    if (UNLIKELY(nInterruptNumber >= 256))
      return false;
    if (UNLIKELY(pHandler != 0 && m_pDbgHandler[nInterruptNumber] != 0))
      return false;
    if (UNLIKELY(pHandler == 0 && m_pDbgHandler[nInterruptNumber] == 0))
      return false;

    // Change the pHandler
    m_pDbgHandler[nInterruptNumber] = pHandler;

    return true;
  }
  size_t HostedInterruptManager::getBreakpointInterruptNumber()
  {
    return 3;
  }
  size_t HostedInterruptManager::getDebugInterruptNumber()
  {
    return 1;
  }

#endif

void HostedInterruptManager::interrupt(InterruptState &interruptState)
{
  size_t nIntNumber = interruptState.getInterruptNumber();

  #if defined(DEBUGGER)
  {
    InterruptHandler *pHandler;

    // Get the debugger handler
    {
      LockGuard<Spinlock> lockGuard(m_Instance.m_Lock);
      pHandler = m_Instance.m_pDbgHandler[nIntNumber];
    }

    // Call the kernel debugger's handler, if any
    if (pHandler != 0)
      pHandler->interrupt(nIntNumber, interruptState);
  }
  #endif

  InterruptHandler *pHandler;

  // Get the interrupt handler
  {
    LockGuard<Spinlock> lockGuard(m_Instance.m_Lock);
    pHandler = m_Instance.m_pHandler[nIntNumber];
  }

  // Call the normal interrupt handler, if any
  if (LIKELY(pHandler != 0))
  {
    pHandler->interrupt(nIntNumber, interruptState);
    return;
  }

  // Were we running in the kernel, or user space?
  // User space processes have a subsystem, kernel ones do not.
#ifdef THREADS
  Thread *pThread = Processor::information().getCurrentThread();
  Process *pProcess = pThread->getParent();
  Subsystem *pSubsystem = pProcess->getSubsystem();
  if(pSubsystem)
  {
      if(UNLIKELY(nIntNumber == 0))
      {
          pSubsystem->threadException(pThread, Subsystem::DivideByZero);
          return;
      }
      else if(UNLIKELY(nIntNumber == 6))
      {
          pSubsystem->threadException(pThread, Subsystem::InvalidOpcode);
          return;
      }
      else if(UNLIKELY(nIntNumber == 13))
      {
          pSubsystem->threadException(pThread, Subsystem::GeneralProtectionFault);
          return;
      }
      else if(UNLIKELY(nIntNumber == 16))
      {
          pSubsystem->threadException(pThread, Subsystem::FpuError);
          return;
      }
      else if(UNLIKELY(nIntNumber == 19))
      {
          pSubsystem->threadException(pThread, Subsystem::SpecialFpuError);
          return;
      }
  }
#endif

  // unhandled interrupt, check for an exception (interrupts 0-31 inclusive are
  // reserved, not for use by system programmers)
  if(LIKELY(nIntNumber < 32 &&
            nIntNumber != 1 &&
            nIntNumber != 3))
  {
    // TODO:: Check for debugger initialisation.
    // TODO: register dump, maybe a breakpoint so the deubbger can take over?
    // TODO: Rework this
    // for now just print out the exception name and number
    static LargeStaticString e;
    e.clear();
    e.append ("Exception #0x");
    e.append (nIntNumber, 16);
    if (nIntNumber == 14)
    {
      uint64_t cr2;
      asm volatile("mov %%cr2, %%rax" : "=a" (cr2));
      e.append(" at 0x");
      e.append(cr2, 16, 16, '0');
    }

    if (nIntNumber == 8)
    {
      // On amd64, we actually have a functional InterruptState.
      ERROR_NOLOCK("(double fault, system is very unhappy)");

      uint64_t cr2;
      asm volatile("mov %%cr2, %%rax" : "=a" (cr2));
      NOTICE_NOLOCK("  -> #DF possibly caused by #PF at " << cr2 << ".");
    }
#if defined(DEBUGGER)
    Debugger::instance().start(interruptState, e);
#else
    panic(e);
#endif
  }
}

//
// Functions only usable in the kernel initialisation phase
//

void HostedInterruptManager::initialiseProcessor()
{
}

HostedInterruptManager::HostedInterruptManager()
  : m_Lock()
{
  // Initialise the pointers to the pHandler
  for (size_t i = 0;i < 256;i++)
  {
    m_pHandler[i] = 0;
    #ifdef DEBUGGER
      m_pDbgHandler[i] = 0;
    #endif
  }
}

HostedInterruptManager::~HostedInterruptManager()
{
}

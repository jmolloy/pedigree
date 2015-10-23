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
#include <process/TimeTracker.h>
#include <Subsystem.h>
#endif

const char* g_ExceptionNames[] =
{
  "Divide Error",
  "Debug",
  "NMI Interrupt",
  "Breakpoint",
  "Overflow",
  "BOUND Range Exceeded",
  "Invalid Opcode",
  "Device Not Available",
  "Double Fault",
  "Coprocessor Segment Overrun", /* recent IA-32 processors don't generate this */
  "Invalid TSS",
  "Segment Not Present",
  "Stack Fault",
  "General Protection Fault",
  "Page Fault",
  "FPU Floating-Point Error",
  "Alignment Check",
  "Machine-Check",
  "SIMD Floating-Point Exception",
  "Reserved: Interrupt 20",
  "Reserved: Interrupt 21",
  "Reserved: Interrupt 22",
  "Reserved: Interrupt 23",
  "Reserved: Interrupt 24",
  "Reserved: Interrupt 25",
  "Reserved: Interrupt 26",
  "Reserved: Interrupt 27",
  "Reserved: Interrupt 28",
  "Reserved: Interrupt 29",
  "Reserved: Interrupt 30",
  "Reserved: Interrupt 31"
};

X64InterruptManager X64InterruptManager::m_Instance;

InterruptManager &InterruptManager::instance()
{
  return X64InterruptManager::instance();
}

bool X64InterruptManager::registerInterruptHandler(size_t nInterruptNumber,
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

  bool X64InterruptManager::registerInterruptHandlerDebugger(size_t nInterruptNumber,
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
  size_t X64InterruptManager::getBreakpointInterruptNumber()
  {
    return 3;
  }
  size_t X64InterruptManager::getDebugInterruptNumber()
  {
    return 1;
  }

#endif

void X64InterruptManager::interrupt(InterruptState &interruptState)
{
  TimeTracker tracker(0, !interruptState.kernelMode());
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
    e.append (": \"");
    e.append (g_ExceptionNames[nIntNumber]);
    e.append ("\"");
    if (nIntNumber == 14)
    {
      uint64_t cr2;
      asm volatile("mov %%cr2, %%rax" : "=a" (cr2));
      e.append(" at 0x");
      e.append(cr2, 16, 16, '0');
      e.append(", errorcode 0x");
      e.append(interruptState.m_Errorcode, 16, 8, '0');
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

void X64InterruptManager::initialiseProcessor()
{
  // Load the IDT
  struct
  {
    uint16_t size;
    uint64_t idt;
  } PACKED idtr = {4095, reinterpret_cast<uintptr_t>(&m_Instance.m_IDT)};

  asm volatile("lidt %0" :: "m"(idtr));
}

void X64InterruptManager::setInterruptGate(size_t nInterruptNumber,
                                           uintptr_t interruptHandler)
{
  m_IDT[nInterruptNumber].offset0 = interruptHandler & 0xFFFF;
  m_IDT[nInterruptNumber].selector = 0x08;
  m_IDT[nInterruptNumber].ist = 0;
  m_IDT[nInterruptNumber].flags = 0xEE /*0x8E*/;
  m_IDT[nInterruptNumber].offset1 = (interruptHandler >> 16) & 0xFFFF;
  m_IDT[nInterruptNumber].offset2 = (interruptHandler >> 32) & 0xFFFFFFFF;
  m_IDT[nInterruptNumber].res = 0;
}

void X64InterruptManager::setIst(size_t nInterruptNumber, size_t ist)
{
  if(ist > 7)
    return;
  m_IDT[nInterruptNumber].ist = ist;
}

X64InterruptManager::X64InterruptManager()
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

  // Initialise the IDT
  extern uintptr_t interrupt_handler_array[];
  for (size_t i = 0;i < 256;i++)
    setInterruptGate(i, interrupt_handler_array[i]);

  // Set double fault handler IST entry.
  setIst(8, 1);
}
X64InterruptManager::~X64InterruptManager()
{
}

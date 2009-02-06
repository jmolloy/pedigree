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

#include <panic.h>
#include <LockGuard.h>
#include <utilities/StaticString.h>
#include <processor/Processor.h>
#include "InterruptManager.h"
#if defined(DEBUGGER)
  #include <Debugger.h>
#endif

#define SYSCALL_INTERRUPT_NUMBER 255

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

X86InterruptManager X86InterruptManager::m_Instance;

SyscallManager &SyscallManager::instance()
{
  return X86InterruptManager::instance();
}
InterruptManager &InterruptManager::instance()
{
  return X86InterruptManager::instance();
}

bool X86InterruptManager::registerInterruptHandler(size_t nInterruptNumber,
                                                   InterruptHandler *pHandler)
{
  LockGuard<Spinlock> lockGuard(m_Lock);

  if (UNLIKELY(nInterruptNumber >= 256 || nInterruptNumber == SYSCALL_INTERRUPT_NUMBER))
    return false;
  if (UNLIKELY(pHandler != 0 && m_pHandler[nInterruptNumber] != 0))
    return false;
  if (UNLIKELY(pHandler == 0 && m_pHandler[nInterruptNumber] == 0))
    return false;

  m_pHandler[nInterruptNumber] = pHandler;
  return true;
}

#ifdef DEBUGGER

  bool X86InterruptManager::registerInterruptHandlerDebugger(size_t nInterruptNumber,
                                                             InterruptHandler *pHandler)
  {
    LockGuard<Spinlock> lockGuard(m_Lock);

    if (UNLIKELY(nInterruptNumber >= 256 || nInterruptNumber == SYSCALL_INTERRUPT_NUMBER))
      return false;
    if (UNLIKELY(pHandler != 0 && m_pDbgHandler[nInterruptNumber] != 0))
      return false;
    if (UNLIKELY(pHandler == 0 && m_pDbgHandler[nInterruptNumber] == 0))
      return false;

    m_pDbgHandler[nInterruptNumber] = pHandler;
    return true;
  }
  size_t X86InterruptManager::getBreakpointInterruptNumber()
  {
    return 3;
  }
  size_t X86InterruptManager::getDebugInterruptNumber()
  {
    return 1;
  }

#endif

bool X86InterruptManager::registerSyscallHandler(Service_t Service, SyscallHandler *pHandler)
{
  LockGuard<Spinlock> lockGuard(m_Lock);

  if (UNLIKELY(Service >= serviceEnd))
    return false;
  if (UNLIKELY(pHandler != 0 && m_pSyscallHandler[Service] != 0))
    return false;
  if (UNLIKELY(pHandler == 0 && m_pSyscallHandler[Service] == 0))
    return false;

  m_pSyscallHandler[Service] = pHandler;
  return true;
}

void X86InterruptManager::interrupt(InterruptState &interruptState)
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

  // Call the syscall handler, if it is the syscall interrupt
  if (nIntNumber == SYSCALL_INTERRUPT_NUMBER)
  {
    size_t serviceNumber = interruptState.getSyscallService();
    
    if (UNLIKELY(serviceNumber >= serviceEnd))
    {
      // TODO: We should return an error here
      return;
    }

    SyscallHandler *pHandler;
  
    // Get the syscall handler
    {
      LockGuard<Spinlock> lockGuard(m_Instance.m_Lock);
      pHandler = m_Instance.m_pSyscallHandler[serviceNumber];
    }

    if (LIKELY(pHandler != 0))
    {
      interruptState.m_Eax = pHandler->syscall(interruptState);
      NOTICE("Returning errno " << Hex << Processor::information().getCurrentThread()->getErrno());
      interruptState.m_Ebx = Processor::information().getCurrentThread()->getErrno();
    }
    return;
  }

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

  // unhandled interrupt, check for an exception (interrupts 0-31 inclusive are
  // reserved, not for use by system programmers)
  // TODO: Rework this
  if(LIKELY(nIntNumber < 32 &&
            nIntNumber != 1 &&
            nIntNumber != 3))
  {
    // TODO:: Check for debugger initialisation.
    // TODO: register dump, maybe a breakpoint so the deubbger can take over?
    // for now just print out the exception name and number
    static LargeStaticString e;
    e.clear();
    e.append ("Exception #");
    e.append (nIntNumber, 10);
    e.append (": \"");
    e.append (g_ExceptionNames[nIntNumber]);
    e.append ("\"");
    if (nIntNumber == 14)
    {
      uint32_t cr2;
      asm volatile("mov %%cr2, %%eax" : "=a" (cr2));
      e.append(" at 0x");
      e.append(cr2, 16, 8, '0');
      e.append(", errorcode 0x");
      e.append(interruptState.m_Errorcode, 16, 8, '0');
    }
#if defined(DEBUGGER)
    Debugger::instance().start(interruptState, e);
#else
    panic(e);
#endif
  }
}

uintptr_t X86InterruptManager::syscall(Service_t service, uintptr_t function, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5)
{
  uint32_t eax = ((service&0xFFFF)<<16) | (function&0xFFFF);
  uintptr_t ret;
  asm volatile("int $255" : "=a" (ret) : "0" (eax), "b" (p1), "c" (p2), "d" (p3), "S" (p4), "D" (p5));
  return ret;
}

//
// Functions only usable in the kernel initialisation phase
//

void X86InterruptManager::initialiseProcessor()
{
  // Load the IDT
  struct
  {
    uint16_t size;
    uint32_t idt;
  } PACKED idtr = {2047, reinterpret_cast<uintptr_t>(&m_Instance.m_IDT)};

  asm volatile("lidt %0" :: "m"(idtr));
}

void X86InterruptManager::setInterruptGate(size_t nInterruptNumber,
                                           uintptr_t interruptHandler,
                                           bool bUserspace)
{
  m_IDT[nInterruptNumber].offset0 = interruptHandler & 0xFFFF;
  m_IDT[nInterruptNumber].selector = 0x08;
  m_IDT[nInterruptNumber].res = 0;
  m_IDT[nInterruptNumber].flags = bUserspace ? 0xEE : 0x8E;
  m_IDT[nInterruptNumber].offset1 = (interruptHandler >> 16) & 0xFFFF;
}

X86InterruptManager::X86InterruptManager()
  : m_Lock()
{
  // Initialise the pointers to the interrupt handler
  for (size_t i = 0;i < 256;i++)
  {
    m_pHandler[i] = 0;
    #if defined(DEBUGGER)
      m_pDbgHandler[i] = 0;
    #endif
  }

  // Initialise the pointers to the syscall handler
  for (size_t i = 0;i < serviceEnd;i++)
    m_pSyscallHandler[i] = 0;

  // Initialise the IDT
  extern uintptr_t interrupt_handler_array[];
  for (size_t i = 0;i < 256;i++)
    setInterruptGate(i,
                     interrupt_handler_array[i],
                     (i == SYSCALL_INTERRUPT_NUMBER || i == 3 /* Interrupt number */) ? true : false);
}
X86InterruptManager::~X86InterruptManager()
{
}

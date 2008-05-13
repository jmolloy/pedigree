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
#include <Debugger.h>
#include <LockGuard.h>
#include <utilities/StaticString.h>
#include "InterruptManager.h"

#define SYSCALL_INTERRUPT_NUMBER 255

const char* g_ExceptionNames[] = {
  "Divide Error [div by 0 OR dest operand too small]",
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
  "Int20_rsvd",
  "Int21_rsvd",
  "Int22_rsvd",
  "Int23_rsvd",
  "Int24_rsvd",
  "Int25_rsvd",
  "Int26_rsvd",
  "Int27_rsvd",
  "Int28_rsvd",
  "Int29_rsvd",
  "Int30_rsvd",
  "Int31_rsvd"
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

bool X86InterruptManager::registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler)
{
  LockGuard lockGuard(m_Lock);

  if (UNLIKELY(interruptNumber >= 256 || interruptNumber == SYSCALL_INTERRUPT_NUMBER))
    return false;
  if (UNLIKELY(handler != 0 && m_Handler[interruptNumber] != 0))
    return false;
  if (UNLIKELY(handler == 0 && m_Handler[interruptNumber] == 0))
    return false;

  m_Handler[interruptNumber] = handler;
  return true;
}

#ifdef DEBUGGER

  bool X86InterruptManager::registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler)
  {
    LockGuard lockGuard(m_Lock);

    if (UNLIKELY(interruptNumber >= 256 || interruptNumber == SYSCALL_INTERRUPT_NUMBER))
      return false;
    if (UNLIKELY(handler != 0 && m_DbgHandler[interruptNumber] != 0))
      return false;
    if (UNLIKELY(handler == 0 && m_DbgHandler[interruptNumber] == 0))
      return false;

    m_DbgHandler[interruptNumber] = handler;
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

bool X86InterruptManager::registerSyscallHandler(Service_t Service, SyscallHandler *handler)
{
  LockGuard lockGuard(m_Lock);

  if (UNLIKELY(Service >= serviceEnd))
    return false;
  if (UNLIKELY(handler != 0 && m_SyscallHandler[Service] != 0))
    return false;
  if (UNLIKELY(handler == 0 && m_SyscallHandler[Service] == 0))
    return false;

  m_SyscallHandler[Service] = handler;
  return true;
}

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

void X86InterruptManager::setInterruptGate(size_t interruptNumber, uintptr_t interruptHandler, bool userspace)
{
  m_IDT[interruptNumber].offset0 = interruptHandler & 0xFFFF;
  m_IDT[interruptNumber].selector = 0x08;
  m_IDT[interruptNumber].res = 0;
  m_IDT[interruptNumber].flags = userspace ? 0xEE : 0x8E;
  m_IDT[interruptNumber].offset1 = (interruptHandler >> 16) & 0xFFFF;
}
void X86InterruptManager::interrupt(InterruptState &interruptState)
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
    // unhandled interrupt, check for an exception (interrupts 0-31 inclusive are
    // reserved, not for use by system programmers)
    if(LIKELY(intNumber < 32 &&
              intNumber != 1 &&
              intNumber != 3))
    {
      // TODO:: Check for debugger initialisation.
      // TODO: register dump, maybe a breakpoint so the deubbger can take over?
      // for now just print out the exception name and number
      static LargeStaticString e;
      e.clear();
      e.append ("Exception #");
      e.append (intNumber, 10);
      e.append (": \"");
      e.append (g_ExceptionNames[intNumber]);
      e.append ("\"");
      if (intNumber == 14)
      {
        uint32_t cr2;
        asm volatile("mov %%cr2, %%eax" : "=a" (cr2));
        e.append(" at 0x");
        e.append(cr2, 16, 8, '0');
        e.append(", errorcode 0x");
        e.append(interruptState.m_Errorcode, 16, 8, '0');
      }
#ifdef DEBUGGER
      Debugger::instance().start(interruptState, e);
#else
      panic(e);
#endif
    }
  }
}

X86InterruptManager::X86InterruptManager()
  : m_Lock()
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

  // Initialise the IDT
  extern uintptr_t interrupt_handler_array[];
  for (size_t i = 0;i < 256;i++)
    setInterruptGate(i,
                     interrupt_handler_array[i],
                     (i == SYSCALL_INTERRUPT_NUMBER) ? true : false);
}
X86InterruptManager::~X86InterruptManager()
{
}

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
#include "interrupt.h"

#define SYSCALL_INTERRUPT_NUMBER 255

X86InterruptManager X86InterruptManager::m_Instance;

SyscallManager &SyscallManager::instance()
{
  return X86InterruptManager::m_Instance;
}
InterruptManager &InterruptManager::instance()
{
  return X86InterruptManager::m_Instance;
}

bool X86InterruptManager::registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler)
{
  // TODO: Needs locking
  if (UNLIKELY(interruptNumber >= 256 || interruptNumber == SYSCALL_INTERRUPT_NUMBER || m_Handler[interruptNumber] != 0))
    return false;

  m_Handler[interruptNumber] = handler;
  return true;
}

#ifdef DEBUGGER

  bool X86InterruptManager::registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler)
  {
    // TODO: Needs locking
    if (UNLIKELY(interruptNumber >= 256 || m_DbgHandler[interruptNumber] != 0))
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
  // TODO
  return true;
}

void X86InterruptManager::initialise()
{
  extern uintptr_t interrupt_handler_array[];
  for (size_t i = 0;i < 256;i++)
    m_Instance.setInterruptGate(i, interrupt_handler_array[i], (i == SYSCALL_INTERRUPT_NUMBER) ? true : false);
}
void X86InterruptManager::initialiseProcessor()
{
  // Load the IDT
  struct
  {
    uint16_t size;
    uint32_t idt;
  } __attribute__((packed)) idtr = {2047, reinterpret_cast<uintptr_t>(&m_Instance.m_IDT)};

  asm volatile("lidt %0" : "=m"(idtr));
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
    // TODO
  }
  // Call the normal interrupt handler, if any, otherwise
  else if (m_Instance.m_Handler[intNumber] != 0)
    m_Instance.m_Handler[intNumber]->interrupt(intNumber, interruptState);
}

X86InterruptManager::X86InterruptManager()
{
}
X86InterruptManager::~X86InterruptManager()
{
}

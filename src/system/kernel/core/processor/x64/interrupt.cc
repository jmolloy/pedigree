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

X64InterruptManager X64InterruptManager::m_Instance;

InterruptManager &InterruptManager::instance()
{
  return X64InterruptManager::instance();
}

bool X64InterruptManager::registerInterruptHandler(size_t interruptNumber, InterruptHandler *handler)
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

  bool X64InterruptManager::registerInterruptHandlerDebugger(size_t interruptNumber, InterruptHandler *handler)
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
  size_t X64InterruptManager::getBreakpointInterruptNumber()
  {
    return 3;
  }
  size_t X64InterruptManager::getDebugInterruptNumber()
  {
    return 1;
  }

#endif

void X64InterruptManager::initialiseProcessor()
{
  // Load the IDT
  struct
  {
    uint16_t size;
    uint64_t idt;
  } PACKED idtr = {4095, reinterpret_cast<uintptr_t>(&m_Instance.m_IDT)};

  asm volatile("lidt %0" : "=m"(idtr));
}

void X64InterruptManager::setInterruptGate(size_t interruptNumber, uintptr_t interruptHandler)
{
  m_IDT[interruptNumber].offset0 = interruptHandler & 0xFFFF;
  m_IDT[interruptNumber].selector = 0x08;
  m_IDT[interruptNumber].flags = 0x8E;
  m_IDT[interruptNumber].offset1 = (interruptHandler >> 16) & 0xFFFF;
  m_IDT[interruptNumber].offset2 = (interruptHandler >> 32) & 0xFFFFFFFF;
  m_IDT[interruptNumber].res = 0;
}
void X64InterruptManager::interrupt(InterruptState &interruptState)
{
  // TODO: Needs locking

  size_t intNumber = interruptState.getInterruptNumber();

  #ifdef DEBUGGER
    // Call the kernel debugger's handler, if any
    if (m_Instance.m_DbgHandler[intNumber] != 0)
      m_Instance.m_DbgHandler[intNumber]->interrupt(intNumber, interruptState);
  #endif

  // Call the normal interrupt handler, if any
  if (LIKELY(m_Instance.m_Handler[intNumber] != 0))
    m_Instance.m_Handler[intNumber]->interrupt(intNumber, interruptState);
}

X64InterruptManager::X64InterruptManager()
{
  // Initialise the pointers to the handler
  for (size_t i = 0;i < 256;i++)
  {
    m_Handler[i] = 0;
    #ifdef DEBUGGER
      m_DbgHandler[i] = 0;
    #endif
  }

  // Initialise the IDT
  extern uintptr_t interrupt_handler_array[];
  for (size_t i = 0;i < 256;i++)
    setInterruptGate(i, interrupt_handler_array[i]);
}
X64InterruptManager::~X64InterruptManager()
{
}

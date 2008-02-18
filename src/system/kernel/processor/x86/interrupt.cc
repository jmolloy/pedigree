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
#include "../x86_common/interrupt.h"

void x86_common::InterruptManager::loadIDT()
{
  struct
  {
    uint16_t size;
    uint32_t idt;
  } __attribute__((packed)) idtr = {2047, reinterpret_cast<uintptr_t>(&m_IDT)};

  // Load the IDT
  asm volatile("lidt %0" : "=m"(idtr));
}

void x86_common::InterruptManager::setInterruptGate(size_t interruptNumber, uintptr_t interruptHandler, bool userspace)
{
  m_IDT[interruptNumber].offset0 = interruptHandler & 0xFFFF;
  m_IDT[interruptNumber].selector = 0x08;
  m_IDT[interruptNumber].res = 0;
  m_IDT[interruptNumber].flags = userspace ? 0xEE : 0x8E;
  m_IDT[interruptNumber].offset1 = (interruptHandler >> 16) & 0xFFFF;
}

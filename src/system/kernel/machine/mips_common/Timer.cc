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
#include <compiler.h>
#include <processor/InterruptManager.h>
#include <machine/Machine.h>
#include <processor/Processor.h>
#include <Log.h>
#include "Timer.h"

CountCompareTimer CountCompareTimer::m_Instance;

bool CountCompareTimer::registerHandler(TimerHandler *handler)
{
  if (UNLIKELY(handler == 0 && m_Handler != 0))
    return false;
  
  m_Handler = handler;
  return true;
}

bool CountCompareTimer::initialise()
{
  // Allocate the Interrupt
  InterruptManager &IntManager = InterruptManager::instance();
  if (IntManager.registerInterruptHandler(0, this) == false)
    return false;

  Processor::setInterrupts(true);

  uint32_t bleh;
  asm volatile("mfc0 %0, $12, 1" : "=r" (bleh));
  NOTICE("Bleh: " << Hex << bleh);

  // Unmask our interrupt vector.
  uint32_t sr;
  asm volatile("mfc0 %0, $12;nop" : "=r" (sr));
//  NOTICE(Hex << sr);
//  Processor::breakpoint();
  sr |= (0xC0 << 8); // Enable external interrupt 0 - first two bits are software ints.
  asm volatile("mtc0 %0, $12;nop" : : "r" (sr));

  // Set up the compare register.
  asm volatile("mtc0 $zero, $11; nop");

  return true;
}
void CountCompareTimer::uninitialise()
{
}

CountCompareTimer::CountCompareTimer()
  : m_Handler(0), m_Compare(0)
{
}

void CountCompareTimer::interrupt(size_t interruptNumber, InterruptState &state)
{
  NOTICE("Interrupt: " << interruptNumber);
  Processor::breakpoint();
  // Set up the compare register.
  m_Compare += 0x100000;
  asm volatile("mtc0 %0, $11; nop" : : "r" (m_Compare));
  // Read count
  uint32_t cause;
  asm volatile("mfc0 %0, $13; nop" : "=r" (cause));
  NOTICE("Cause: " << Hex << cause);
  // TODO: Delta is wrong
  if (LIKELY(m_Handler != 0))
    m_Handler->timer(0, state);

}

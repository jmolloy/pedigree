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

#include <compiler.h>
#include <processor/InterruptManager.h>
#include "../core/processor/mips32/InterruptManager.h"
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
  // Allocate the Interrupt.
  MIPS32InterruptManager &IntManager = static_cast<MIPS32InterruptManager&> (InterruptManager::instance());
  if (IntManager.registerExternalInterruptHandler(7, this) == false)
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
  sr |= (0x80 << 8); // Enable external interrupt 0 - first two bits are software ints.
  asm volatile("mtc0 %0, $12;nop" : : "r" (sr));
  

  // Set up the compare register.
  asm volatile("mtc0 %0, $11; nop" : : "r" (m_Compare));
  // Zero the count register.
  asm volatile("mtc0 $zero, $9; nop");

  return true;
}
void CountCompareTimer::uninitialise()
{
}

CountCompareTimer::CountCompareTimer()
  : m_Handler(0), m_Compare(0x10000)
{
}

void CountCompareTimer::interrupt(size_t interruptNumber, InterruptState &state)
{
  // Set up the compare register.
  m_Compare += 0x10000;
  asm volatile("mtc0 %0, $11; nop" : : "r" (m_Compare));

//  Processor::breakpoint();

  // Read count - are we about to overlap?
  uint32_t count;
  asm volatile("mfc0 %0, $9; nop" : "=r" (count));

  if (count > m_Compare)
  {
    asm volatile("mtc0 %0, $9; nop" : : "r" (m_Compare-0x10000));
  }

  // TODO: Delta is wrong
  if (LIKELY(m_Handler != 0))
    m_Handler->timer(0, state);

}

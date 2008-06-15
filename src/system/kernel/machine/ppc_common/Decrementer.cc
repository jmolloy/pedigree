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
#include "../core/processor/ppc32/InterruptManager.h"
#include <machine/Machine.h>
#include <processor/Processor.h>
#include <Log.h>
#include <machine/openfirmware/OpenFirmware.h>
#include <machine/openfirmware/Device.h>
#include "Decrementer.h"

Decrementer Decrementer::m_Instance;

bool Decrementer::registerHandler(TimerHandler *handler)
{
  if (UNLIKELY(handler == 0 && m_Handler != 0))
    return false;
  
  m_Handler = handler;
  return true;
}

bool Decrementer::initialise()
{
  // Allocate the Interrupt.
  if (InterruptManager::instance().registerInterruptHandler(8, this) == false)
    return false;

  // Find the frequency of the decrementer.
  OFDevice chosen (OpenFirmware::instance().findDevice("/chosen"));
  OFDevice cpu (chosen.getProperty("cpu"));
  m_Frequency = reinterpret_cast<uint32_t> (cpu.getProperty("timebase-frequency"));
  if (m_Frequency == -1)
  {
    WARNING("Cpu::timebase-frequency property not available!");
    m_Frequency = 0x1000000; // 16MHz
  }

  // Set up the decrementer to fire in DECREMENTER_PERIOD milliseconds.
  uint32_t n = (DECREMENTER_PERIOD * m_Frequency) / 1000;
  asm volatile("mtdec %0" : : "r" (n));

  return true;
}
void Decrementer::uninitialise()
{
}

Decrementer::Decrementer()
  : m_Handler(0), m_Frequency(0)
{
}

void Decrementer::interrupt(size_t interruptNumber, InterruptState &state)
{
  Processor::breakpoint();

  // TODO: Delta is wrong
  if (LIKELY(m_Handler != 0))
    m_Handler->timer(0, state);

}

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
#include <machine/Machine.h>
#include <Log.h>
#include "Pit.h"

Pit Pit::m_Instance;

bool Pit::registerHandler(TimerHandler *handler)
{
  if (UNLIKELY(handler == 0 && m_Handler != 0))
    return false;
  
  m_Handler = handler;
  return true;
}

bool Pit::initialise()
{
  // Allocate the PIT I/O range
  if (m_IoPort.allocate(0x40, 4) == false)
    return false;

  // Allocate the IRQ
  IrqManager &irqManager = *Machine::instance().getIrqManager();
  m_IrqId = irqManager.registerIsaIrqHandler(0, this);
  if (m_IrqId == 0)
    return false;

  // TODO: Set the PIT frequency

  return true;
}
void Pit::uninitialise()
{
  // TODO: Reset the PIT frequency

  // Free the IRQ
  if (m_IrqId != 0)
  {
    IrqManager &irqManager = *Machine::instance().getIrqManager();
    irqManager.unregisterHandler(m_IrqId, this);
  }

  // Free the PIT I/O range
  m_IoPort.free();
}

Pit::Pit()
  : m_IoPort("PIT"), m_IrqId(0), m_Handler(0)
{
}

bool Pit::irq(irq_id_t number, InterruptState &state)
{
  ProcessorState procState(state);
  // TODO: Delta is wrong
  if (LIKELY(m_Handler != 0))
    m_Handler->timer(0, procState);

  return true;
}

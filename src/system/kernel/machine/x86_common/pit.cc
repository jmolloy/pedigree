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
#include "pit.h"

Pit Pit::m_Instance;

bool Pit::registerHandler(TimerHandler *handler)
{
  // TODO
}

bool Pit::initialise()
{
  // Allocate the PIT I/O range
  if (m_IoPort.allocate(0x40, 4) == false)
    return false;

  // Allocate the IRQ
  IrqManager &irqManager = IrqManager::instance();
  m_IrqId = irqManager.registerIsaIrqHandler(0, this);
  if (m_IrqId == 0)
    return false;

  return true;
}
void Pit::uninitialise()
{
  // TODO
}

Pit::Pit()
  : m_IoPort(), m_IrqId(0)
{
  // TODO
}

void Pit::irq(irq_id_t number)
{
  // TODO
}

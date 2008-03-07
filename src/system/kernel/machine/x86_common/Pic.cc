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
#include <Log.h>
#include <Debugger.h>
#include <machine/pc/Pic.h>

// TODO: Needs locking

#define BASE_INTERRUPT_VECTOR 0x20

Pic Pic::m_Instance;

irq_id_t Pic::registerIsaIrqHandler(uint8_t irq, IrqHandler *handler)
{
  if (UNLIKELY(irq >= 16))
    return 0;

  // Save the IrqHandler
  m_Handler[irq] = handler;

  // Enable/Unmask the IRQ
  enable(irq, true);

  return irq + BASE_INTERRUPT_VECTOR;
}
irq_id_t Pic::registerPciIrqHandler(IrqHandler *handler)
{
  // TODO
  return 0;
}
void Pic::acknowledgeIrq(irq_id_t Id)
{
  uint8_t irq = Id - BASE_INTERRUPT_VECTOR;

  // Enable the irq again (the interrupt reason got removed)
  enable(irq, true);
}
void Pic::unregisterHandler(irq_id_t Id, IrqHandler *handler)
{
  uint8_t irq = Id - BASE_INTERRUPT_VECTOR;

  // Disable the IRQ
  enable(irq, false);

  // Remove the handler
  m_Handler[irq] = 0;
}

bool Pic::initialise()
{
  // Allocate the I/O ports
  if (m_SlavePort.allocate(0xA0, 4) == false)
    return false;
  if (m_MasterPort.allocate(0x20, 4) == false)
    return false;

  // Initialise the slave and master PIC
  m_MasterPort.write8(0x11, 0);
  m_SlavePort.write8(0x11, 0);
  m_MasterPort.write8(BASE_INTERRUPT_VECTOR, 1);
  m_SlavePort.write8(BASE_INTERRUPT_VECTOR + 0x08, 1);
  m_MasterPort.write8(0x04, 1);
  m_SlavePort.write8(0x02, 1);
  m_MasterPort.write8(0x01, 1);
  m_SlavePort.write8(0x01, 1);

  // Register the interrupts
  InterruptManager &IntManager = InterruptManager::instance();
  for (size_t i = 0;i < 16;i++)
    if (IntManager.registerInterruptHandler(i + BASE_INTERRUPT_VECTOR, this) == false)
      return false;

  // Disable all IRQ's (exept IRQ2)
  enableAll(false);

  return true;
}

Pic::Pic()
  : m_SlavePort(), m_MasterPort()
{
  for (size_t i = 0;i < 16;i++)
    m_Handler[i] = 0;
}

void Pic::interrupt(size_t interruptNumber, InterruptState &state)
{
  size_t irq = (interruptNumber - BASE_INTERRUPT_VECTOR);

  // Is Spurios IRQ7?
  if (irq == 7)
  {
    m_MasterPort.write8(0x03, 3);
    if (UNLIKELY((m_MasterPort.read8(0) & 0x80) == 0))
    {
      NOTICE("PIC: spurious IRQ7");
      return;
    }
  }
  // Is spurious IRQ15?
  else if (irq == 15)
  {
    m_SlavePort.write8(0x03, 3);
    if (UNLIKELY((m_SlavePort.read8(0) & 0x80) == 0))
    {
      NOTICE("PIC: spurious IRQ15");
      return;
    }
  }

  // Call the irq handler, if any
  if (LIKELY(m_Handler[irq] != 0))
  {
    if (m_Handler[irq]->irq(irq) == false)
    {
      // Disable/Mask the IRQ line (the handler did not remove
      // the interrupt reason, yet)
      enable(irq, false);
    }
  }
  else
  {
    NOTICE("PIC: unhandled irq #" << irq << " occurred");
    Debugger::instance().breakpoint(state);
  }

  eoi(irq);
}

void Pic::eoi(uint8_t irq)
{
  m_MasterPort.write8(0x20, 0);
  if (irq > 7)
    m_SlavePort.write8(0x20, 0);
}
void Pic::enable(uint8_t irq, bool enable)
{
  if (irq <= 7)
  {
    uint8_t mask = m_MasterPort.read8(1);
    if (enable == true)mask = mask & ~(1 << irq);
    else mask = mask | (1 << irq);

    m_MasterPort.write8(mask, 1);
  }
  else
  {
    uint8_t mask = m_SlavePort.read8(1);
    if (enable == true)mask = mask & ~(1 << (irq - 8));
    else mask = mask | (1 << (irq - 8));

    m_SlavePort.write8(mask, 1);
  }
}
void Pic::enableAll(bool enable)
{
  if (enable == false)
  {
    m_MasterPort.write8(0xFB, 1);
    m_SlavePort.write8(0xFB, 1);
  }
  else
  {
    m_MasterPort.write8(0x00, 1);
    m_SlavePort.write8(0x00, 1);
  }
}

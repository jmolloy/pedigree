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
#include <Log.h>
#include <Debugger.h>
#include <processor/Processor.h>
#include <machine/Device.h>
#include <machine/openfirmware/Device.h>
#include "Heathrow.h"

// TODO: Needs locking

Heathrow Heathrow::m_Instance;

irq_id_t Heathrow::registerIsaIrqHandler(uint8_t irq, IrqHandler *handler)
{
  // Save the IrqHandler
  m_Handler[irq] = handler;

  // Enable/Unmask the IRQ
  enable(irq, true);

  return irq;
}
irq_id_t Heathrow::registerPciIrqHandler(IrqHandler *handler)
{
  // TODO
  return 0;
}
void Heathrow::acknowledgeIrq(irq_id_t Id)
{
  uint8_t irq = Id;

  // Enable the irq again (the interrupt reason got removed)
  enable(irq, true);
  eoi(irq);
}
void Heathrow::unregisterHandler(irq_id_t Id, IrqHandler *handler)
{
  uint8_t irq = Id;

  // Disable the IRQ
  enable(irq, false);

  // Remove the handler
  m_Handler[irq] = 0;
}

void Heathrow::searchNode(Device *pDev)
{
  for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
  {
    Device *pChild = pDev->getChild(i);
    OFDevice ofDev (pChild->getOFHandle());
    NormalStaticString type;
    ofDev.getProperty("device_type", type);
    if (type == "interrupt-controller")
    {
      // Found it - double check that it's the right compatibility.
      NormalStaticString comp;
      ofDev.getProperty("compatible", comp);
      if (!(comp == "heathrow"))
      {
        return;
      }

      uint32_t reg[2];
      ofDev.getProperty("reg", reg, 8);
      
      // Look for the parent's BAR0.
      for (unsigned int j = 0; j < pDev->addresses().count(); j++)
      {
        if (!strcmp(pDev->addresses()[j]->m_Name, "bar0"))
        {
          uintptr_t regAddr = static_cast<uintptr_t>(reg[0]) + pDev->addresses()[j]->m_Address;
          size_t regSize = 0x40;//static_cast<size_t>(reg[1]);
          pChild->addresses().pushBack(
            new Device::Address(String("reg"),
                                regAddr,
                                regSize,
                                false /* Always memory for PPC */));
          m_pPort = pChild->addresses()[j]->m_Io;
          return;
        }
      }
    }
    // Recurse.
    if (m_pPort) return;
      searchNode(pChild);
  }
}

bool Heathrow::initialise()
{
  // Allocate the I/O ports - run through the device tree until we find a device
  // with a property "interrupt-controller".
  Device *dev = &Device::root();
  searchNode(dev);

  // Did the search return anything?
  if (!m_pPort)
  {
    ERROR("Heathrow: Device not found!");
    return false;
  }
  
  // Register the interrupts
  InterruptManager &IntManager = InterruptManager::instance();
  if (IntManager.registerInterruptHandler(4, this) == false)
    return false;

  m_nIrqs = 64;
  
  // Disable all IRQ's
  enableAll(false);

  return true;
}

Heathrow::Heathrow()
  : m_pPort(0), m_nIrqs(0), m_LowMask(0), m_HighMask(0)
{
  for (size_t i = 0;i < 16;i++)
    m_Handler[i] = 0;
}

void Heathrow::interrupt(size_t interruptNumber, InterruptState &state)
{
  uint32_t low = LITTLE_TO_HOST32(m_pPort->read32(0x10)) & m_LowMask;
  uint32_t high = LITTLE_TO_HOST32(m_pPort->read32(0x00)) & m_HighMask;

  int32_t irq = -1;
  for (int i = 0; i < 32; i++)
  {
    if (low & (1<<i))
    {
      irq = i;
      break;
    }
  }
  if (irq == -1)
    for (int i = 0; i < 32; i++)
    {
      if (high & (1<<i))
      {
        irq = i+32;
        break;
      }
    }

  if (irq == -1)
    return;

  // size_t irq = (interruptNumber - BASE_INTERRUPT_VECTOR);

  // // Is Spurios IRQ7?
  // if (irq == 7)
  // {
  //   m_MasterPort.write8(0x03, 3);
  //   if (UNLIKELY((m_MasterPort.read8(0) & 0x80) == 0))
  //   {
  //     NOTICE("PIC: spurious IRQ7");
  //     eoi(irq);
  //     return;
  //   }
  // }
  // /// \todo Logic faulty here, reporting spurious interrupts for disk accesses!
  // // Is spurious IRQ15?
  // else if (irq == 15)
  // {
  //   m_SlavePort.write8(0x03, 3);
  //   if (UNLIKELY((m_SlavePort.read8(0) & 0x80) == 0))
  //   {
  //     NOTICE("PIC: spurious IRQ15");
  //     eoi(irq);
  //     return;
  //   }
  // }

  // Call the irq handler, if any
  if (LIKELY(m_Handler[irq] != 0))
  {
    if (m_Handler[irq]->irq(irq, state) == false)
    {
      // Disable/Mask the IRQ line (the handler did not remove
      // the interrupt reason, yet)
      enable(irq, false);
    }
  }
  else
  {
    NOTICE("PIC: unhandled irq #" << irq << " occurred");
 
    // #ifdef DEBUGGER
    //   LargeStaticString str;
    //   str += "Unhandled IRQ: #";
    //   str += irq;
    //   str += " occurred.";
    //   Debugger::instance().start(state, str);
    // #endif
    enable(irq, false);
  }

  eoi(irq);
}

void Heathrow::eoi(uint8_t irq)
{
  uintptr_t reg = 0x18;
  if (irq > 31)
  {
    reg = 0x08;
    irq -= 32;
  }
  
  uint32_t shift = 1<<irq;
  m_pPort->write32(LITTLE_TO_HOST32(shift), reg);
}
void Heathrow::enable(uint8_t irq, bool enable)
{
  uintptr_t reg = 0x14;
  if (irq > 31)
  {
    reg = 0x04;
    irq -= 32;
  }
  
  uint32_t value = LITTLE_TO_HOST32(m_pPort->read32(reg));
  uint32_t shift = 1<<irq;
  if (enable) value |= shift; else value &= ~shift;

  if (reg == 0x14) m_LowMask = value; else m_HighMask = value;

  m_pPort->write32(LITTLE_TO_HOST32(value), reg);
}
void Heathrow::enableAll(bool enable)
{
  if (enable)
  {
    m_pPort->write32(0xFFFFFFFF, 0x04);
    m_pPort->write32(0xFFFFFFFF, 0x14);
    m_LowMask = 0xFFFFFFFF;
    m_HighMask = 0xFFFFFFFF;
  } else {
    m_pPort->write32(0x00000000, 0x04);
    m_pPort->write32(0x00000000, 0x14);
    m_LowMask = 0;
    m_HighMask = 0;
  }
}

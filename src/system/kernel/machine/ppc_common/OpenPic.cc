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
#include <machine/Device.h>
#include <machine/openfirmware/Device.h>
#include "OpenPic.h"

// TODO: Needs locking

OpenPic OpenPic::m_Instance;

irq_id_t OpenPic::registerIsaIrqHandler(uint8_t irq, IrqHandler *handler)
{
  if (UNLIKELY(irq >= 64))
    return 0;

  // Save the IrqHandler
  m_Handler[irq] = handler;

  // Enable/Unmask the IRQ
  enable(irq, true);

  return irq;
}
irq_id_t OpenPic::registerPciIrqHandler(IrqHandler *handler)
{
  // TODO
  return 0;
}
void OpenPic::acknowledgeIrq(irq_id_t Id)
{
  uint8_t irq = Id;

  // Enable the irq again (the interrupt reason got removed)
  enable(irq, true);
  eoi(irq);
}
void OpenPic::unregisterHandler(irq_id_t Id, IrqHandler *handler)
{
  uint8_t irq = Id;

  // Disable the IRQ
  enable(irq, false);

  // Remove the handler
  m_Handler[irq] = 0;
}

void OpenPic::searchNode(Device *pDev)
{
  for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
  {
    Device *pChild = pDev->getChild(i);
    OFDevice ofDev (pChild->getOFHandle());
    NormalStaticString type;
    ofDev.getProperty("device_type", type);
    if (type == "open-pic")
    {
      uint32_t reg[2];
      ofDev.getProperty("reg", reg, 8);
      
      // Look for the parent's BAR0.
      for (unsigned int j = 0; j < pDev->addresses().count(); j++)
      {
        if (!strcmp(pDev->addresses()[j]->m_Name, "bar0"))
        {
          uintptr_t regAddr = static_cast<uintptr_t>(reg[0]) + pDev->addresses()[j]->m_Address;
          size_t regSize = static_cast<size_t>(reg[1]);

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

bool OpenPic::initialise()
{
  // Allocate the I/O ports - run through the device tree until we find a device
  // with a property "interrupt-controller".
  Device *dev = &Device::root();
  searchNode(dev);

  // Did the search return anything?
  if (!m_pPort)
  {
    ERROR("OpenPic: Device not found!");
    return false;
  }
  
  // Disable 8259 passthrough mode.
  m_pPort->write32(HOST_TO_LITTLE32(OPENPIC_FLAG_CONF0_P), OPENPIC_REG_CONF0);

  // Register the interrupts
  InterruptManager &IntManager = InterruptManager::instance();
  if (IntManager.registerInterruptHandler(4, this) == false)
    return false;

  // Grab the feature reporting register.
  uint32_t feature = LITTLE_TO_HOST32(m_pPort->read32(OPENPIC_REG_FEATURE));
  Feature f = *reinterpret_cast<Feature*>(&feature);

  // Save the number of valid IRQ sources, and check the version.
  m_nIrqs = f.num_irq+1;
  if (f.version > 0x2)
  {
    ERROR("OpenPIC: invalid version returned: " << Hex << f.version);
    return false;
  }

  /// \todo Set up timers.

  // Set up all IRQ sources.
  for (int i = 0; i < m_nIrqs; i++)
  {
    uintptr_t reg = i*0x20 + OPENPIC_SOURCE_START;

    // Set up interrupt sources.
    m_pPort->write32(HOST_TO_LITTLE32(OPENPIC_SOURCE_MASK | OPENPIC_SOURCE_PRIORITY | i), reg);
    // Ensure that IRQs are sent to this processor (the first one).
#ifdef MULTIPROCESSOR
#error Problems here.
#endif
    m_pPort->write32(HOST_TO_LITTLE32(0x1), reg+0x10);
  }

  asm volatile("sync");

  // Set the task priority to 0, so all interrupts with priority >0 can be received.
  m_pPort->write32(0x0, OPENPIC_REG_TASK_PRIORITY);

  return true;
}

OpenPic::OpenPic()
  : m_pPort(0), m_nIrqs(0)
{
  for (size_t i = 0;i < 64;i++)
    m_Handler[i] = 0;
}

void OpenPic::interrupt(size_t interruptNumber, InterruptState &state)
{
  uint32_t irq = LITTLE_TO_HOST32(m_pPort->read32(OPENPIC_REG_ACK));

  // Is Spurious IRQ255?
  if (irq == 0xff)
  {
    NOTICE("PIC: spurious IRQ255");
    eoi(irq);
    return;
  }

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
 
    #ifdef DEBUGGER
      LargeStaticString str;
      str += "Unhandled IRQ: #";
      str += irq;
      str += " occurred.";
      Debugger::instance().start(state, str);
    #endif
  }

  eoi(irq);
}

void OpenPic::eoi(uint8_t irq)
{
  m_pPort->write32(0, OPENPIC_REG_EOI);
}
void OpenPic::enable(uint8_t irq, bool enable)
{
  uintptr_t reg = irq*0x20 + OPENPIC_SOURCE_START;

  uintptr_t receivedReg = LITTLE_TO_HOST32(m_pPort->read32(reg));

  if (enable)
    m_pPort->write32(HOST_TO_LITTLE32(receivedReg & ~OPENPIC_SOURCE_MASK) , reg);
  else
    m_pPort->write32(HOST_TO_LITTLE32(receivedReg | OPENPIC_SOURCE_MASK) , reg);

  asm volatile("sync; isync");
}
void OpenPic::enableAll(bool enable)
{
  for (int i = 0; i < m_nIrqs; i++)
    this->enable(i, enable);
}

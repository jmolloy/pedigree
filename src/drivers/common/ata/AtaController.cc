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
#include "AtaController.h"
#include <Log.h>
#include <machine/Machine.h>
#include <processor/Processor.h>

AtaController::AtaController(Controller *pDev) :
  Controller(pDev), m_pCommandRegs(0), m_pControlRegs(0)
{
  setSpecificType(String("ata-controller"));

  // Ensure we have no stupid children lying around.
  m_Children.clear();

  // Initialise our ports.
  for (unsigned int i = 0; i < m_Addresses.count(); i++)
  {
    /// \todo String operator== problem here.
    if (!strcmp(m_Addresses[i]->m_Name, "command") || !strcmp(m_Addresses[i]->m_Name, "bar0"))
      m_pCommandRegs = m_Addresses[i]->m_Io;
    if (!strcmp(m_Addresses[i]->m_Name, "control") || !strcmp(m_Addresses[i]->m_Name, "bar1"))
      m_pControlRegs = m_Addresses[i]->m_Io;
  }
  
  // Create two disks - master and slave.
  AtaDisk *pMaster = new AtaDisk(this, true);
  AtaDisk *pSlave = new AtaDisk(this, false);

  // Try and initialise the disks.  
  bool masterInitialised = pMaster->initialise();
  bool slaveInitialised = pSlave->initialise();

  // Only add a child disk if it initialised properly.
  if (masterInitialised)
    addChild(pMaster);
  else
    delete pMaster;

  if (slaveInitialised)
    addChild(pSlave);
  else
    delete pSlave;

  Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler*> (this));
  initialise();

}

AtaController::~AtaController()
{
}

uint64_t AtaController::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                                       uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8)
{
  AtaDisk *pDisk = reinterpret_cast<AtaDisk*> (p2);
  if (p1 == ATA_CMD_READ)
    return pDisk->doRead(p3, p4, static_cast<uintptr_t> (p5));
  else
    return pDisk->doWrite(p3, p4, static_cast<uintptr_t> (p5));
}

bool AtaController::irq(irq_id_t number, InterruptState &state)
{
  for (unsigned int i = 0; i < getNumChildren(); i++)
  {
    AtaDisk *pDisk = static_cast<AtaDisk*> (getChild(i));
    pDisk->irqReceived();
  }
  return false; // Keep the IRQ disabled - level triggered.
}

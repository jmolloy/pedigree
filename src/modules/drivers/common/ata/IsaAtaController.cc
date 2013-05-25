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
#include "IsaAtaController.h"
#include <Log.h>
#include <machine/Machine.h>
#include <processor/Processor.h>

IsaAtaController::IsaAtaController(Controller *pDev, int nController) :
  AtaController(pDev, nController)
{
  setSpecificType(String("ata-controller"));

  // Initialise our ports.
  bool bPortsFound = false;
  for (unsigned int i = 0; i < m_Addresses.count(); i++)
  {
    if (m_Addresses[i]->m_Name == "command" || m_Addresses[i]->m_Name == "bar0")
    {
      m_pCommandRegs = m_Addresses[i]->m_Io;
      bPortsFound = true;
    }
    if (m_Addresses[i]->m_Name == "control" || m_Addresses[i]->m_Name == "bar1")
    {
      m_pControlRegs = m_Addresses[i]->m_Io;
      bPortsFound = true;
    }
  }
  
  if(!bPortsFound)
  {
    ERROR("ISA ATA: No addresses found for this controller");
    return;
  }

  // Look for a floating bus
  if(m_pControlRegs->read8(6) == 0xFF || m_pCommandRegs->read8(7) == 0xFF)
  {
      // No devices on this controller
      return;
  }
  
  m_Children.clear();
  
  // Set up the RequestQueue
  initialise();

  // Perform a software reset.
  m_pControlRegs->write8(0x04, 6); // Assert SRST
  Semaphore dammitWeNeedABloodySleepFunction(0); // We really do!
  dammitWeNeedABloodySleepFunction.acquire(1, 0, 5000);

  m_pControlRegs->write8(0, 6); // Negate SRST
  Semaphore dammitWeNeedABloodySleepFunctionAndThisSemaphoreIsRedundantBecauseWeDontHaveOne(0); // We really do!
  dammitWeNeedABloodySleepFunctionAndThisSemaphoreIsRedundantBecauseWeDontHaveOne.acquire(1, 0, 5000);

  // Poll until BSY is clear. Until BSY is clear, no other bits in the
  // alternate status register are considered valid.
  uint8_t status = 0;
  while(1) // ((status&0xC0) != 0) && ((status&0x9) == 0) )
  {
    status = m_pControlRegs->read8(6);
    if(status & 0x80)
        continue;
    else if(status & 0x1)
    {
        NOTICE("Error during ATA software reset, status = " << status);
        return;
    }
    else
        break;

  }

  // Create two disks - master and slave.
  AtaDisk *pMaster = new AtaDisk(this, true, m_pCommandRegs, m_pControlRegs);
  AtaDisk *pSlave = new AtaDisk(this, false, m_pCommandRegs, m_pControlRegs);

  // Try and initialise the disks.
  bool masterInitialised = pMaster->initialise();
  bool slaveInitialised = pSlave->initialise();

  Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler*> (this));

  // Initialise potential ATAPI disks (add as children before initialising so they get IRQs)
  if (masterInitialised)
    addChild(pMaster);
  else
  {
    delete pMaster;
    AtapiDisk *pMasterAtapi = new AtapiDisk(this, true, m_pCommandRegs, m_pControlRegs);
    addChild(pMasterAtapi);
    if(!pMasterAtapi->initialise())
    {
      removeChild(pMasterAtapi);
      delete pMasterAtapi;
    }
  }

  if (slaveInitialised)
    addChild(pSlave);
  else
  {
    delete pSlave;
    AtapiDisk *pSlaveAtapi = new AtapiDisk(this, false, m_pCommandRegs, m_pControlRegs);
    addChild(pSlaveAtapi);
    if(!pSlaveAtapi->initialise())
    {
      removeChild(pSlaveAtapi);
      delete pSlaveAtapi;
    }
  }
}

IsaAtaController::~IsaAtaController()
{
}

uint64_t IsaAtaController::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
                                       uint64_t p5, uint64_t p6, uint64_t p7, uint64_t p8)
{
  AtaDisk *pDisk = reinterpret_cast<AtaDisk*> (p2);
  if(p1 == ATA_CMD_READ)
    return pDisk->doRead(p3);
  else if(p1 == ATA_CMD_WRITE)
    return pDisk->doWrite(p3);
  else
    return 0;
}

bool IsaAtaController::irq(irq_id_t number, InterruptState &state)
{
  for (unsigned int i = 0; i < getNumChildren(); i++)
  {
    AtaDisk *pDisk = static_cast<AtaDisk*> (getChild(i));
    if(pDisk->isAtapi())
    {
        AtapiDisk *pAtapiDisk = static_cast<AtapiDisk*>(pDisk);
        pAtapiDisk->irqReceived();
    }
    else
        pDisk->irqReceived();
  }
  return false; // Keep the IRQ disabled - level triggered.
}

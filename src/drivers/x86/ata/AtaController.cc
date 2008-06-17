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

AtaController::AtaController(Controller *pDev) :
  Controller(pDev), m_CommandRegs("ATA Controller: command registers"),
  m_ControlRegs("ATA Controller: control registers")
{
  // Initialise our ports.
  for (int i = 0; i < m_Addresses.count(); i++)
  {
    /// \todo String operator== problem here.
    if (!strcmp(m_Addresses[i]->m_Name, "command"))
    {
      NOTICE("Command: " << m_Addresses[i]->m_Address);
      m_CommandRegs.allocate(m_Addresses[i]->m_Address, m_Addresses[i]->m_Size);
    }
    if (!strcmp(m_Addresses[i]->m_Name, "control"))
      m_ControlRegs.allocate(m_Addresses[i]->m_Address, m_Addresses[i]->m_Size);
  }
  NOTICE("AtaController(), " << Hex << (uint32_t)pDev << ", this " << (uint32_t)this);
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

}

AtaController::~AtaController()
{
}

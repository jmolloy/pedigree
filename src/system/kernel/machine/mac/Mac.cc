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

#include "Mac.h"
#include <machine/openfirmware/OpenFirmware.h>
#include <machine/openfirmware/Device.h>
#include <processor/Processor.h>
#include <machine/Device.h>
#include <machine/ppc_common/pci.h>
#include <machine/ppc_common/OpenPic.h>
#include <machine/ppc_common/Heathrow.h>
#include <Log.h>

extern size_t resolveInterruptNumber(Device *pDev);

Mac Mac::m_Instance;

Machine &Machine::instance()
{
  return Mac::instance();
}

void Mac::initialise()
{
  m_Vga.initialise();
  m_Keyboard.initialise();
  m_Decrementer.initialise();
  m_bInitialised = true;
}
Serial *Mac::getSerial(size_t n)
{
  return &m_Serial[n];
}
size_t Mac::getNumSerial()
{
  return 1;
}
Vga *Mac::getVga(size_t n)
{
  return &m_Vga;
}
size_t Mac::getNumVga()
{
  return 1;
}
IrqManager *Mac::getIrqManager()
{
  return m_pIrqManager;
}
SchedulerTimer *Mac::getSchedulerTimer()
{
  return &m_Decrementer;
}
Timer *Mac::getTimer()
{
  // TODO
  return 0;
}
Keyboard *Mac::getKeyboard()
{
  return &m_Keyboard;
}

Mac::Mac() :
  m_Decrementer(), m_Vga(), m_Keyboard(), m_pIrqManager(0)
{
}
Mac::~Mac()
{
}

// Performs a depth-first probe of the (OpenFirmware) device tree, populating the internal device tree.
static void probeDev(int depth, OFDevice *pOfDev, Device *pInternalDev)
{
  OFHandle hChild = OpenFirmware::instance().getFirstChild(pOfDev);
  while (hChild != 0)
  {
    // Convert the device handle into a real device object.
    OFDevice dChild (hChild);
    OFHandle hOldChild = hChild;
    // Set hChild to now be a handle for the next device to probe.
    hChild = OpenFirmware::instance().getSibling(&dChild);

    // Populate the new internal device node.
    NormalStaticString type;
    dChild.getProperty("device_type", type);

    // Prune stupid items.
    if (type.length() == 0)
      continue;

    // Create a new internal device node.
    Device *node = new Device();
    pInternalDev->addChild(node);
    node->setParent(pInternalDev);

    node->setSpecificType(String(type));
    node->setOFHandle(hOldChild);
    node->setInterruptNumber(resolveInterruptNumber(node));

    probeDev(depth+1, &dChild, node);
  }
}

void Mac::initialiseDeviceTree()
{
  // Find the root devices.
  OFDevice ofRoot( OpenFirmware::instance().findDevice("/") );
  Device &internalRoot = Device::root();
  
  // Start the device probing - this is recursive and obviously starts at a depth of 0.
  probeDev(0, &ofRoot, &internalRoot);

  initialisePci();
  if (!OpenPic::instance().initialise())
  {
    // OpenPic not found - try heathrow.
    if (!Heathrow::instance().initialise())
    {
      ERROR ("No IRQ manager found.");
    }
    else
    {
      m_pIrqManager = &Heathrow::instance();
    }
  }
  else
  {
    m_pIrqManager = &OpenPic::instance();
  }
}

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

#include "PciAtaController.h"
#include <processor/Processor.h>
#include <machine/Machine.h>
#include <machine/Pci.h>
#include <Log.h>

PciAtaController::PciAtaController(Controller *pDev, int nController) :
    AtaController(pDev, nController), m_PciControllerType(UnknownController)
{
    setSpecificType(String("ata-controller"));

    // Determine controller type
    NormalStaticString str;
    switch(getPciDeviceId())
    {
        case 0x1230:
            m_PciControllerType = PIIX;
            str = "PIIX";
            break;
        case 0x7010:
            m_PciControllerType = PIIX3;
            str = "PIIX3";
            break;
        case 0x7111:
            m_PciControllerType = PIIX4;
            str = "PIIX4";
            break;
        case 0x2411:
            m_PciControllerType = ICH;
            str = "ICH";
            break;
        case 0x2421:
            m_PciControllerType = ICH0;
            str = "ICH0";
            break;
        case 0x244A:
        case 0x244B:
            m_PciControllerType = ICH2;
            str = "ICH2";
            break;
        case 0x248A:
        case 0x248B:
            m_PciControllerType = ICH3;
            str = "ICH3";
            break;
        case 0x24CA:
        case 0x24CB:
            m_PciControllerType = ICH4;
            str = "ICH4";
            break;
        case 0x24DB:
            m_PciControllerType = ICH5;
            str = "ICH5";
            break;
        default:
            m_PciControllerType = UnknownController;
            str = "<unknown>";
            break;
    }
    str += " PCI IDE controller found";
    NOTICE(static_cast<const char*>(str));

    if(m_PciControllerType == UnknownController)
        return;
    
    // Find BAR4 (BusMaster registers)
    Device::Address *bar4 = 0;
    for(size_t i = 0; i < addresses().count(); i++)
    {
        if(!strcmp(static_cast<const char *>(addresses()[i]->m_Name), "bar4"))
            bar4 = addresses()[i];
    }

    m_Children.clear();

    // Set up the RequestQueue
    initialise();

    // Read the BusMaster interface base address register and tell it where we
    // would like to talk to it (BAR4).
    uint32_t busMasterIfaceAddr = PciBus::instance().readConfigSpace(pDev, 8);
    busMasterIfaceAddr &= 0xFFFF000F;
    busMasterIfaceAddr |= bar4->m_Address & 0xFFF0;
    NOTICE("    - Bus master interface base register at " << bar4->m_Address);
    PciBus::instance().writeConfigSpace(pDev, 8, busMasterIfaceAddr);

    // Read the command register and then enable I/O space. We do this so that
    // we can still access drives using PIO. We also enable the BusMaster
    // function on the controller.
    uint32_t commandReg = PciBus::instance().readConfigSpace(pDev, 1);
    commandReg = (commandReg & ~(0x5)) | 0x5;
    PciBus::instance().writeConfigSpace(pDev, 1, commandReg);

    // Fiddle with the IDE timing registers
    uint32_t ideTiming = PciBus::instance().readConfigSpace(pDev, 0x10);
    ideTiming = 0xCCE0 | (3 << 4) | 4;
    ideTiming |= (ideTiming << 16);
    PciBus::instance().writeConfigSpace(pDev, 0x10, ideTiming);

    // Write the interrupt line into the PCI space if needed.
    uint32_t miscFields = PciBus::instance().readConfigSpace(pDev, 0xF);
    if((miscFields & 0xF) != getInterruptNumber())
    {
        if(getInterruptNumber())
        {
            NOTICE("    - Configured PCI space for interrupt line " << getInterruptNumber());
            miscFields &= ~0xF;
            miscFields |= getInterruptNumber() & 0xF;
        }
    }
    PciBus::instance().writeConfigSpace(pDev, 0xF, miscFields);

    // The controller must be able to perform BusMaster IDE DMA transfers, or
    // else we have to fall back to PIO transfers.
    bool bDma = false;
    if(pDev->getPciProgInterface() & 0x80)
    {
        NOTICE("    - This is a DMA capable controller");
        bDma = true;
    }

    IoPort *masterCommand = new IoPort("pci-ide-master-cmd");
    IoPort *slaveCommand = new IoPort("pci-ide-slave-cmd");
    IoPort *masterControl = new IoPort("pci-ide-master-ctl");
    IoPort *slaveControl = new IoPort("pci-ide-slave-ctl");

    /// \todo Bus master registerss may be memory mapped...
    IoPort *bar4_a = 0;
    IoPort *bar4_b = 0;
    BusMasterIde *primaryBusMaster = 0;
    BusMasterIde *secondaryBusMaster = 0;
    if(bDma)
    {
        uintptr_t addr = bar4->m_Address;

        // Okay, now delete the BAR's IoBase. We're not going to use it again,
        // and we need the ports.
        delete bar4->m_Io;
        bar4->m_Io = 0;

        bar4_a = new IoPort("pci-ide-busmaster-primary");
        if(!bar4_a->allocate(addr, 8))
        {
            ERROR("Couldn't allocate primary BusMaster ports");
            delete bar4_a;
            bar4_a = 0;
        }

        bar4_b = new IoPort("pci-ide-busmaster-secondary");
        if(!bar4_b->allocate(addr + 8, 8))
        {
            ERROR("Couldn't allocate secondary BusMaster ports");
            delete bar4_b;
            bar4_b = 0;
        }

        // Create the BusMasterIde objects
        if(bar4_a)
        {
            primaryBusMaster = new BusMasterIde;
            if(!primaryBusMaster->initialise(bar4_a))
            {
                ERROR("Couldn't initialise primary BusMaster IDE interface");
                delete primaryBusMaster;
                delete bar4_a;

                primaryBusMaster = 0;
            }
        }
        if(bar4_b)
        {
            secondaryBusMaster = new BusMasterIde;
            if(!secondaryBusMaster->initialise(bar4_b))
            {
                ERROR("Couldn't initialise secondary BusMaster IDE interface");
                delete secondaryBusMaster;
                delete bar4_b;

                secondaryBusMaster = 0;
            }
        }
    }

    // By default, this is the port layout we can expect for the system
    /// \todo ICH will have "native mode" to worry about
    if(!masterCommand->allocate(0x1F0, 8))
        ERROR("Couldn't allocate master command ports");
    if(!masterControl->allocate(0x3F4, 4))
        ERROR("Couldn't allocate master control ports");
    if(!slaveCommand->allocate(0x170, 8))
        ERROR("Couldn't allocate slave command ports");
    if(!slaveControl->allocate(0x374, 4))
        ERROR("Couldn't allocate slave control ports");

    // Install our IRQ handler
    if(getInterruptNumber() != 0xFF)
        Machine::instance().getIrqManager()->registerIsaIrqHandler(getInterruptNumber(), static_cast<IrqHandler*> (this));

    // And finally, create disks
    diskHelper(true, masterCommand, masterControl, primaryBusMaster);
    diskHelper(false, masterCommand, masterControl, primaryBusMaster);
    diskHelper(true, slaveCommand, slaveControl, secondaryBusMaster);
    diskHelper(false, slaveCommand, slaveControl, secondaryBusMaster);
}

PciAtaController::~PciAtaController()
{
}

void PciAtaController::diskHelper(bool master, IoBase *cmd, IoBase *ctl, BusMasterIde *dma)
{
    AtaDisk *pDisk = new AtaDisk(this, master, cmd, ctl, dma);
    if(!pDisk->initialise())
    {
        delete pDisk;
        AtapiDisk *pAtapiDisk = new AtapiDisk(this, master, cmd, ctl, dma);

        addChild(pAtapiDisk);

        if(!pAtapiDisk->initialise())
        {
            removeChild(pAtapiDisk);
            delete pAtapiDisk;
        }
    }
    else
        addChild(pDisk);
}

uint64_t PciAtaController::executeRequest(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4,
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

bool PciAtaController::irq(irq_id_t number, InterruptState &state)
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

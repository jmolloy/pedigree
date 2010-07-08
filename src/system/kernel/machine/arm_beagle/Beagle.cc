/*
 * Copyright (c) 2010 Matthew Iselin
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

#include "Beagle.h"
#include "Prcm.h"
#include "I2C.h"
#include "Gpio.h"

#include <machine/Device.h>
#include <machine/Bus.h>
#include <machine/Disk.h>
#include <machine/Controller.h>
#include <machine/Pci.h>

ArmBeagle ArmBeagle::m_Instance;

Machine &Machine::instance()
{
  return ArmBeagle::instance();
}

void ArmBeagle::initialiseDeviceTree()
{
    Bus *pL4 = new Bus("L4-Interconnect");
    pL4->setSpecificType(String("l4"));

    Controller *pGpio1 = new Controller();
    pGpio1->setSpecificType(String("gpio"));
    pGpio1->addresses().pushBack(new Device::Address(String("mmio"), 0x48310000, 0x1000, false));
    pGpio1->setInterruptNumber(29);
    pL4->addChild(pGpio1);
    pGpio1->setParent(pL4);

    Controller *pGpio2 = new Controller();
    pGpio2->setSpecificType(String("gpio"));
    pGpio2->addresses().pushBack(new Device::Address(String("mmio"), 0x49050000, 0x1000, false));
    pGpio2->setInterruptNumber(30);
    pL4->addChild(pGpio2);
    pGpio2->setParent(pL4);

    Controller *pGpio3 = new Controller();
    pGpio3->setSpecificType(String("gpio"));
    pGpio3->addresses().pushBack(new Device::Address(String("mmio"), 0x49052000, 0x1000, false));
    pGpio3->setInterruptNumber(31);
    pL4->addChild(pGpio3);
    pGpio3->setParent(pL4);

    Controller *pGpio4 = new Controller();
    pGpio4->setSpecificType(String("gpio"));
    pGpio4->addresses().pushBack(new Device::Address(String("mmio"), 0x49054000, 0x1000, false));
    pGpio4->setInterruptNumber(32);
    pL4->addChild(pGpio4);
    pGpio4->setParent(pL4);

    Controller *pGpio5 = new Controller();
    pGpio5->setSpecificType(String("gpio"));
    pGpio5->addresses().pushBack(new Device::Address(String("mmio"), 0x49056000, 0x1000, false));
    pGpio5->setInterruptNumber(33);
    pL4->addChild(pGpio5);
    pGpio5->setParent(pL4);

    Controller *pGpio6 = new Controller();
    pGpio6->setSpecificType(String("gpio"));
    pGpio6->addresses().pushBack(new Device::Address(String("mmio"), 0x49058000, 0x1000, false));
    pGpio6->setInterruptNumber(34);
    pL4->addChild(pGpio6);
    pGpio6->setParent(pL4);

    Controller *pMmc1 = new Controller();
    pMmc1->setSpecificType(String("mmc_sd_sdio"));
    pMmc1->addresses().pushBack(new Device::Address(String("mmio"), 0x4809C000, 0x1000, false));
    pMmc1->setInterruptNumber(83);
    pL4->addChild(pMmc1);
    pMmc1->setParent(pL4);

    Controller *pMmc2 = new Controller();
    pMmc2->setSpecificType(String("mmc_sd_sdio"));
    pMmc2->addresses().pushBack(new Device::Address(String("mmio"), 0x480B4000, 0x1000, false));
    pMmc2->setInterruptNumber(86);
    pL4->addChild(pMmc2);
    pMmc2->setParent(pL4);

    Controller *pMmc3 = new Controller();
    pMmc3->setSpecificType(String("mmc_sd_sdio"));
    pMmc3->addresses().pushBack(new Device::Address(String("mmio"), 0x480AD000, 0x1000, false));
    pMmc3->setInterruptNumber(94);
    pL4->addChild(pMmc3);
    pMmc3->setParent(pL4);

    Controller *pCtl = new Controller();
    pCtl->setSpecificType(String("display-subsys"));
    pCtl->addresses().pushBack(new Device::Address(String("DSI Protocol Engine"), 0x4804FC00, 512, false));
    pCtl->addresses().pushBack(new Device::Address(String("DSI_PHY"), 0x4804FE00, 64, false));
    pCtl->addresses().pushBack(new Device::Address(String("DSI PLL Controller"), 0x4804FF00, 32, false));
    pCtl->addresses().pushBack(new Device::Address(String("Display Subsystem"), 0x48050000, 512, false));
    pCtl->addresses().pushBack(new Device::Address(String("Display Controller"), 0x48050400, 1024, false));
    pCtl->addresses().pushBack(new Device::Address(String("RFBI"), 0x48050800, 256, false));
    pCtl->addresses().pushBack(new Device::Address(String("Video Encoder"), 0x48050C00, 256, false));
    pCtl->setInterruptNumber(25);
    pL4->addChild(pCtl);
    pCtl->setParent(pL4);

#if 1
    pCtl = new Controller();
    pCtl->setSpecificType(String("ehci-controller"));
    pCtl->setPciIdentifiers(0x0C, 0x03, 0, 0, 0x20); // EHCI PCI identifiers
    pCtl->addresses().pushBack(new Device::Address(String("mmio"), 0x48064800, 1024, false));
    pCtl->setInterruptNumber(77);
    pL4->addChild(pCtl);
    pCtl->setParent(pL4);
#else
    pCtl = new Controller();
    pCtl->setSpecificType(String("ohci-controller"));
    pCtl->setPciIdentifiers(0x0C, 0x03, 0, 0, 0x10); // OHCI PCI identifiers
    pCtl->addresses().pushBack(new Device::Address(String("mmio"), 0x48064400, 1024, false));
    pCtl->setInterruptNumber(76);
    pL4->addChild(pCtl);
    pCtl->setParent(pL4);
#endif

    Device::root().addChild(pL4);
    pL4->setParent(&Device::root());
}

void ArmBeagle::initialise()
{
  m_Serial[0].setBase(0x49020000); // uart3, RS-232 output on board
  //m_Serial[1].setBase(0x4806A000); // uart1
  //m_Serial[2].setBase(0x4806C000); // uart2

  m_bInitialised = true;
}
void ArmBeagle::initialise2()
{
  extern SyncTimer g_SyncTimer;

  Prcm::instance().initialise(0x48004000);

  g_SyncTimer.initialise(0x48320000);

  m_Timers[0].initialise(0, 0x48318000);
  m_Timers[1].initialise(1, 0x49032000);
  m_Timers[2].initialise(2, 0x49034000);
  m_Timers[3].initialise(3, 0x49036000);
  m_Timers[4].initialise(4, 0x49038000);
  m_Timers[5].initialise(5, 0x4903A000);
  m_Timers[6].initialise(6, 0x4903C000);
  m_Timers[7].initialise(7, 0x4903E000);
  m_Timers[8].initialise(8, 0x49040000);
  m_Timers[9].initialise(9, 0x48086000);
  m_Timers[10].initialise(10, 0x48088000);

  I2C::instance(0).initialise(0x48070000);
  I2C::instance(1).initialise(0x48072000);
  I2C::instance(2).initialise(0x48060000);

  Gpio::instance().initialise(0x48310000,
                              0x49050000,
                              0x49052000,
                              0x49054000,
                              0x49056000,
                              0x49058000);
}
Serial *ArmBeagle::getSerial(size_t n)
{
  return &m_Serial[n];
}
size_t ArmBeagle::getNumSerial()
{
  return 1; // 3 UARTs attached, only one initialised for now
}
Vga *ArmBeagle::getVga(size_t n)
{
  return 0; // &m_Vga;
}
size_t ArmBeagle::getNumVga()
{
  return 0;
}
IrqManager *ArmBeagle::getIrqManager()
{
  // TODO
  return 0;
}
SchedulerTimer *ArmBeagle::getSchedulerTimer()
{
  return &m_Timers[1];
}
Timer *ArmBeagle::getTimer()
{
  return &m_Timers[1];
}

Keyboard *ArmBeagle::getKeyboard()
{
  return &m_Keyboard;
}

ArmBeagle::ArmBeagle()
{
}
ArmBeagle::~ArmBeagle()
{
}


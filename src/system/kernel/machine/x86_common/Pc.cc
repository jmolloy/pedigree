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

#include "Pc.h"
#include <Log.h>
#include <panic.h>
#if defined(ACPI)
  #include "Acpi.h"
#endif
#if defined(SMP)
  #include "Smp.h"
#endif
#if defined(APIC)
  #include "Apic.h"
#endif
#include <machine/Device.h>
#include <machine/Bus.h>
#include <machine/Disk.h>
#include <machine/Controller.h>
#include <machine/Pci.h>

Pc Pc::m_Instance;

void Pc::initialise()
{
  // Initialise Vga
  if (m_Vga.initialise() == false)
    panic("Pc: Vga initialisation failed");

  // Initialise ACPI
  #if defined(ACPI)
    Acpi &acpi = Acpi::instance();
    acpi.initialise();
  #endif

  // Initialise SMP
  #if defined(SMP)
    Smp &smp = Smp::instance();
    smp.initialise();
  #endif

  // Check for a local APIC
  #if defined(APIC)

    // Physical address of the local APIC
    uint64_t localApicAddress;

    // Get the Local APIC address & I/O APIC list from either the ACPI or the SMP tables
    bool bLocalApicValid = false;
    #if defined(ACPI)
      if ((bLocalApicValid = acpi.validApicInfo()) == true)
        localApicAddress = acpi.getLocalApicAddress();
    #endif
    #if defined(SMP)
      if (bLocalApicValid == false && (bLocalApicValid = smp.valid()) == true)
        localApicAddress = smp.getLocalApicAddress();
    #endif

    // Initialise the local APIC, if we have gotten valid data from
    // the ACPI/SMP structures
    if (bLocalApicValid == true &&
        m_LocalApic.initialise(localApicAddress))
    {
      NOTICE("Local APIC initialised");
    }

  #endif

  // Check for an I/O APIC
  #if defined(APIC)

    // TODO: Check for I/O Apic
    // TODO: Initialise the I/O Apic
    // TODO: IMCR?
    // TODO: Mask the PICs?
    if (false)
    {
    }

    // Fall back to dual 8259 PICs
    else
    {
  #endif

      NOTICE("Falling back to dual 8259 PIC Mode");

      // Initialise PIC
      Pic &pic = Pic::instance();
      if (pic.initialise() == false)
        panic("Pc: Pic initialisation failed");

  #if defined(APIC)
    }
  #endif

  // Initialise serial ports.
  m_pSerial[0].setBase(0x3F8);
  m_pSerial[1].setBase(0x2F8);

  // Initialse the Real-time Clock / CMOS
  Rtc &rtc = Rtc::instance();
  if (rtc.initialise() == false)
    panic("Pc: Rtc initialisation failed");

  // Initialise the PIT
  Pit &pit = Pit::instance();
  if (pit.initialise() == false)
    panic("Pc: Pit initialisation failed");

  m_Keyboard = new X86Keyboard(0x60);
  m_Keyboard->initialise();

  // Find and parse the SMBIOS tables
  #if defined(SMBIOS)
    m_SMBios.initialise();
  #endif

  m_bInitialised = true;
}
#if defined(MULTIPROCESSOR)
  void Pc::initialiseProcessor()
  {
    // TODO: we might need to initialise per-processor ACPI shit, no idea atm

    // Initialise the local APIC
    if (m_LocalApic.initialiseProcessor() == false)
      panic("Pc::initialiseProcessor(): Failed to initialise the local APIC");
  }
#endif

void Pc::initialiseDeviceTree()
{
  // Firstly add the ISA bus.
  Bus *pIsa = new Bus("ISA");
  pIsa->setSpecificType(String("isa"));

  // ATA controllers.
  Controller *pAtaMaster = new Controller();
  pAtaMaster->setSpecificType(String("ata"));
  pAtaMaster->addresses().pushBack(new Device::Address(String("command"), 0x1F0, 8, true));
  pAtaMaster->addresses().pushBack(new Device::Address(String("control"), 0x3F0, 8, true));
  pAtaMaster->setInterruptNumber(14);
  pIsa->addChild(pAtaMaster);
  pAtaMaster->setParent(pIsa);

  Controller *pAtaSlave = new Controller();
  pAtaMaster->setSpecificType(String("ata"));
  pAtaSlave->addresses().pushBack(new Device::Address(String("command"), 0x170, 8, true));
  pAtaSlave->addresses().pushBack(new Device::Address(String("control"), 0x370, 8, true));
  pAtaSlave->setInterruptNumber(15);
  pIsa->addChild(pAtaSlave);
  pAtaSlave->setParent(pIsa);

  // PS/2
  Controller *pPs2Base = new Controller();
  pPs2Base->setSpecificType(String("ps2"));
  pPs2Base->addresses().pushBack(new Device::Address(String("ps2-base"), 0x60, 5, true));
  pPs2Base->setInterruptNumber(1); // 12 for mouse, handled by the driver
  pIsa->addChild(pPs2Base);
  pPs2Base->setParent(pIsa);

  Device::root().addChild(pIsa);
  pIsa->setParent(&Device::root());

  // Initialise the PCI interface
  PciBus::instance().initialise();
}

Serial *Pc::getSerial(size_t n)
{
  return &m_pSerial[n];
}

size_t Pc::getNumSerial()
{
  return 2;
}

Vga *Pc::getVga(size_t n)
{
  return &m_Vga;
}

size_t Pc::getNumVga()
{
  return 1;
}

IrqManager *Pc::getIrqManager()
{
  return &Pic::instance();
}

SchedulerTimer *Pc::getSchedulerTimer()
{
#ifdef MULTIPROCESSOR
  return &m_LocalApic;
#else
  return &Pit::instance();
#endif
}

Timer *Pc::getTimer()
{
  return &Rtc::instance();
}

Keyboard *Pc::getKeyboard()
{
  return m_Keyboard;
}

void Pc::setKeyboard(Keyboard *kb)
{
    m_Keyboard = kb;
}

#ifdef MULTIPROCESSOR
  void Pc::stopAllOtherProcessors()
  {
    m_LocalApic.interProcessorInterruptAllExcludingThis(IPI_HALT_VECTOR,
                                                        0 /* Fixed delivery mode */);
  }
#endif

Pc::Pc()
  : m_Vga(0x3C0, 0xB8000), m_Keyboard(0)
  #if defined(SMBIOS)
    , m_SMBios()
  #endif
  #if defined(APIC)
    , m_LocalApic()
  #endif
{
}
Pc::~Pc()
{
}

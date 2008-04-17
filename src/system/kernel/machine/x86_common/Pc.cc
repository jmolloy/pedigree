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

Pc Pc::m_Instance;

void Pc::initialise()
{
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

  #if defined(APIC)

    uint64_t localApicAddress;
    Vector<IoApicInformation*> *ioApics;

    // Get the Local APIC address & I/O APIC list from either the ACPI or the SMP tables
    bool bApicValid = false;
    #if defined(ACPI)
      if (acpi.validApicInfo() == true)
      {
        localApicAddress = acpi.getLocalApicAddress();
        ioApics = &acpi.getIoApicList();
        bApicValid = true;
      }
    #endif
    #if defined(ACPI) && defined(SMP)
      else
    #endif
    #if defined(SMP)
      if (smp.valid() == true)
      {
        localApicAddress = smp.getLocalApicAddress();
        ioApics = &smp.getIoApicList();
        bApicValid = true;
      }
    #endif

    // Initialise the local APIC, if we have gotten valid data from
    // the ACPI/SMP structures
    Apic apic;
    if (bApicValid == true && 
        apic.initialise(localApicAddress) == true)
    {
      // TODO: initialise local APIC

      // TODO: Check for I/O Apic
      // TODO: Initialise the I/O Apic
      // TODO: IMCR?
      // TODO: Mask the PICs?
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

  // Initialise Vga
  if (m_Vga.initialise() == false)
    panic("Pc: Vga initialisation failed");

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

  m_Keyboard.initialise();

  // Find and parse the SMBIOS tables
  #if defined(SMBIOS)
    m_SMBios.initialise();
  #endif

  m_bInitialised = true;
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
  return &Pit::instance();
}

Timer *Pc::getTimer()
{
  return &Rtc::instance();
}

Keyboard *Pc::getKeyboard()
{
  return &m_Keyboard;
}

Pc::Pc()
  : m_Vga(0x3C0, 0xB8000), m_Keyboard(0x60)
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

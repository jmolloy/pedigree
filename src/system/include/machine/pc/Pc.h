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
#ifndef MACHINE_PC_PC_H
#define MACHINE_PC_PC_H

#include <machine/Machine.h>
#include <machine/pc/Pic.h>
#include <machine/pc/Rtc.h>
#include <machine/pc/Pit.h>
#include <machine/pc/X86Serial.h>
#include <machine/pc/X86Vga.h>
#include <machine/pc/X86Ethernet.h>
#include <Log.h>

/**
 * Concretion of the abstract Machine class for a MIPS Malta board.
 */
class Pc : public virtual Machine
{
public:
  /**
   * Default constructor, does nothing.
   */
  Pc()
  {
  }
  /**
   * Virtual destructor, does nothing.
   */
  virtual ~Pc()
  {
  }
  
  /**
   * Returns true if the machine believes that it is present.
   */
  virtual bool probe()
  {
    return true;
  }
  
  /**
   * Initialises the machine.
   */
  virtual void initialise()
  {
      // Initialise PIC
    Pic &pic = Pic::instance();
    if (pic.initialise() == false)
    {
    // TODO: Do something
      NOTICE("initialiseMachine(): failed 1");
    }

  // Initialse the Real-time Clock / CMOS
    Rtc &rtc = Rtc::instance();
    if (rtc.initialise() == false)
    {
    // TODO: Do something
      NOTICE("initialiseMachine(): failed 2");
    }

  // Initialise the PIT
    Pit &pit = Pit::instance();
    if (pit.initialise() == false)
    {
    // TODO: Do something
      NOTICE("initialiseMachine(): failed 3");
    }
  }
  
  /**
   * Returns the n'th Serial device.
   */
  virtual Serial *getSerial(size_t n)
  {
    return 0;
  }
  
  /**
   * Returns the number of Serial device.
   */
  virtual size_t getNumSerial()
  {
    return 0;
  }
  
  /**
   * Returns the n'th VGA device.
   */
  virtual Vga *getVga(size_t n)
  {
    return 0;
  }
  
  /**
   * Returns the number of VGA devices.
   */
  virtual size_t getNumVga()
  {
    return 0;
  }
  
  /**
   * Returns the n'th Ethernet device.
   */
  virtual Ethernet *getEthernet(size_t n)
  {
    return 0;
  }
  
  /**
   * Returns the number of Ethernet devices.
   */
  virtual size_t getNumEthernet()
  {
    return 0;
  }
  
  /**
   * Returns the n'th SchedulerTimer device.
   */
  virtual SchedulerTimer *getSchedulerTimer(size_t n)
  {
    return &Pit::instance();
  }
  
  /**
   * Returns the number of SchedulerTimer devices.
   */
  virtual size_t getNumSchedulerTimer()
  {
    return 1;
  }
  
  /**
   * Returns the n'th Timer device.
   */
  virtual Timer *getTimer(size_t n)
  {
    return &Rtc::instance();
  }
  
  /**
   * Returns the number of Timer devices.
   */
  virtual size_t getNumTimer()
  {
    return 1;
  }

private:
  X86Serial m_pSerial[2];
  X86Ethernet m_Ethernet;
  X86Vga m_Vga;
};

#endif

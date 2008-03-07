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
#ifndef MACHINE_AU1500_DB1500_H
#define MACHINE_AU1500_DB1500_H

#include <machine/Machine.h>

/**
 * Concretion of the abstract Machine class for a MIPS Malta board.
 */
class Db1500 : public virtual Machine
{
public:
  /**
   * Default constructor, does nothing.
   */
  Db1500()
  {
  }
  /**
   * Virtual destructor, does nothing.
   */
  virtual ~Db1500()
  {
  }
  
  /**
   * Returns true if the machine believes that it is present.
   */
  virtual bool probe()
  {
    return false;
  }
  
  /**
   * Initialises the machine.
   */
  virtual void initialise()
  {
  }
  
  /**
   * Returns the n'th Serial device.
   */
  virtual Serial &getSerial(size_t n)
  {
    return m_pSerial[0];
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
  virtual Vga &getVga(size_t n)
  {
    return m_Vga;
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
  virtual Ethernet &getEthernet(size_t n)
  {
    return m_Ethernet;
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
  virtual SchedulerTimer &getSchedulerTimer(size_t n)
  {
    return m_SchedulerTimer;
  }
  
  /**
   * Returns the number of SchedulerTimer devices.
   */
  virtual size_t getNumSchedulerTimer()
  {
    return 0;
  }
  
  /**
   * Returns the n'th Timer device.
   */
  virtual Timer &getTimer(size_t n)
  {
    return m_Timer;
  }
  
  /**
   * Returns the number of Timer devices.
   */
  virtual size_t getNumTimer()
  {
    return 0;
  }

private:
  Serial m_pSerial[2];
  Ethernet m_Ethernet;
  SchedulerTimer m_SchedulerTimer;
  Timer m_Timers;
  Vga m_Vga;
};

#endif

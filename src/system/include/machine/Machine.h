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
#ifndef MACHINE_MACHINE_H
#define MACHINE_MACHINE_H

#include <compiler.h>
#include <processor/types.h>
#include <machine/Serial.h>
#include <machine/Vga.h>
#include <machine/Ethernet.h>
#include <machine/SchedulerTimer.h>
#include <machine/Timer.h>

/**
 * This is an abstraction on a machine, or board.
 * It provides functions to retrieve Timers, Serial controllers,
 * VGA controllers, Ethernet controllers etc, without having to 
 * know the exact implementation required or memory map.
 * It also provides a "probe" function, which will attempt to detect
 * if a machine is present.
 */
class Machine
{
public:
  Machine();
  /**
   * Virtual destructor, does nothing.
   */
  virtual ~Machine();
  
  /**
   * Returns true if the machine believes that it is present.
   */
  virtual bool probe() =0;
  
  /**
   * Initialises the machine.
   */
  virtual void initialise() =0;
  
  /**
   * Returns the n'th Serial device.
   */
  virtual Serial *getSerial(size_t n) =0;
  
  /**
   * Returns the number of Serial device.
   */
  virtual size_t getNumSerial() =0;
  
  /**
   * Returns the n'th VGA device.
   */
  virtual Vga *getVga(size_t n) =0;
  
  /**
   * Returns the number of VGA devices.
   */
  virtual size_t getNumVga() =0;
  
  /**
   * Returns the n'th Ethernet device.
   */
  virtual Ethernet *getEthernet(size_t n) =0;
  
  /**
   * Returns the number of Ethernet devices.
   */
  virtual size_t getNumEthernet() =0;
  
  /**
   * Returns the n'th SchedulerTimer device.
   */
  virtual SchedulerTimer *getSchedulerTimer(size_t n) =0;
  
  /**
   * Returns the number of SchedulerTimer devices.
   */
  virtual size_t getNumSchedulerTimer() =0;
  
  /**
   * Returns the n'th Timer device.
    */
  virtual Timer *getTimer(size_t n) =0;
  
  /**
   * Returns the number of Timer devices.
   */
  virtual size_t getNumTimer() =0;
};

#endif

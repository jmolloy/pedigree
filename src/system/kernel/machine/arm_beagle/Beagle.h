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

#ifndef MACHINE_ARM_BEAGLE_H
#define MACHINE_ARM_BEAGLE_H

#include <machine/Machine.h>
#include "Serial.h"
#include "Vga.h"
#include "Keyboard.h"
#include "GPTimer.h"

/**
 * Concretion of the abstract Machine class for an ArmBeagle board.
 */
class ArmBeagle : public Machine
{
public:
  inline static Machine &instance()
  {
    return m_Instance;
  }

  virtual void initialise();
  virtual void initialise2();
  virtual Serial *getSerial(size_t n);
  virtual size_t getNumSerial();
  virtual Vga *getVga(size_t n);
  virtual size_t getNumVga();
  virtual IrqManager *getIrqManager();
  virtual SchedulerTimer *getSchedulerTimer();
  virtual Timer *getTimer();
  virtual Keyboard *getKeyboard();
  virtual void setKeyboard(Keyboard *kb) {};

  virtual void initialiseDeviceTree();

private:
  /**
   * Default constructor, does nothing.
   */
  ArmBeagle();
  ArmBeagle(const ArmBeagle &);
  ArmBeagle &operator = (const ArmBeagle &);
  /**
   * Virtual destructor, does nothing.
   */
  virtual ~ArmBeagle();

  ArmBeagleSerial m_Serial[3];
  GPTimer m_Timers[11];
  ArmBeagleVga m_Vga;
  ArmBeagleKeyboard m_Keyboard;

  static ArmBeagle m_Instance;
};

#endif

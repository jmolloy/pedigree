/*
 * 
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifndef MACHINE_MALTA_BONITO64_H
#define MACHINE_MALTA_BONITO64_H

#include <machine/Machine.h>
#include "../Serial.h"
#include "../Vga.h"
#include "../Keyboard.h"

/**
 * Concretion of the abstract Machine class for a MIPS Malta/Bonito64 board.
 */
class Bonito64 : public Machine
{
public:
  inline static Bonito64 &instance() {return m_Instance;}

  virtual void initialise();
  virtual Serial *getSerial(size_t n);
  virtual size_t getNumSerial();
  virtual Vga *getVga(size_t n);
  virtual size_t getNumVga();
  virtual IrqManager *getIrqManager();
  virtual SchedulerTimer *getSchedulerTimer();
  virtual Timer *getTimer();
  virtual Keyboard *getKeyboard();

private:
  /**
   * Default constructor, does nothing.
   */
  Bonito64();
  Bonito64(const Bonito64 &);
  Bonito64 &operator = (const Bonito64 &);
  /**
   * Virtual destructor, does nothing.
   */
  virtual ~Bonito64();

  MaltaSerial m_Serial[2];
  //SchedulerTimer m_SchedulerTimer;
  //Timer m_Timer;
  MaltaVga m_Vga;
  MaltaKeyboard m_Keyboard;

  static Bonito64 m_Instance;
};

#endif

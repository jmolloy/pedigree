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

#ifndef MACHINE_MAC_MAC_H
#define MACHINE_MAC_MAC_H

#include <machine/Machine.h>
#include "../ppc_common/Serial.h"
#include "../ppc_common/Vga.h"
#include "../ppc_common/Keyboard.h"
#include "../ppc_common/Decrementer.h"

/**
 * Concretion of the abstract Machine class for a MIPS Malta board.
 */
class Mac : public Machine
{
public:
  inline static Machine &instance(){return m_Instance;}

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
  Mac();
  Mac(const Mac &);
  Mac &operator = (const Mac &);
  /**
   * Virtual destructor, does nothing.
   */
  virtual ~Mac();

  PPCSerial m_Serial[2];
  Decrementer m_Decrementer;
  //Timer m_Timers;
  PPCVga m_Vga;
  PPCKeyboard m_Keyboard;

  static Mac m_Instance;
};

#endif

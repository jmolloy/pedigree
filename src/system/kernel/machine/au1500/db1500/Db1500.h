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
#include "../Serial.h"
#include "../Vga.h"

class Db1500 : public Machine
{
public:
  inline static Db1500 &instande(){return m_Instance;}

  virtual void initialise();
  virtual bool isInitialised();
  virtual Serial *getSerial(size_t n);
  virtual size_t getNumSerial();
  virtual Vga *getVga(size_t n);
  virtual size_t getNumVga();
  virtual IrqManager &getIrqManager();
  virtual SchedulerTimer &getSchedulerTimer();
  virtual Timer &getTimer();

private:
  /**
   * Default constructor, does nothing.
   */
  Db1500();
  Db1500(const Db1500 &);
  Db1500 &operator = (const Db1500);
  /**
   * Virtual destructor, does nothing.
   */
  virtual ~Db1500();

  bool bInitialised;

  Au1500Serial m_Serial[2];
  // SchedulerTimer m_SchedulerTimer;
  // Timer m_Timer;
  Au1500Vga m_Vga;

  static Db1500 m_Instance;
};

#endif

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

Pc Pc::m_Instance;

void Pc::initialise()
{
  // Initialise serial ports.
  m_pSerial[0].setBase(0x3F8);
  m_pSerial[1].setBase(0x2F8);
  
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
  return 0;
}

size_t Pc::getNumVga()
{
  return 0;
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

Pc::Pc()
{
}
Pc::~Pc()
{
}

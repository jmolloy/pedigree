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

#include "Beagle.h"

ArmBeagle ArmBeagle::m_Instance;

Machine &Machine::instance()
{
  return ArmBeagle::instance();
}

void ArmBeagle::initialise()
{
//  m_Serial[0].setBase(0x101f1000);
  m_bInitialised = true;
}
Serial *ArmBeagle::getSerial(size_t n)
{
  return &m_Serial[n];
}
size_t ArmBeagle::getNumSerial()
{
  return 3; // 3 UARTs attached.
}
Vga *ArmBeagle::getVga(size_t n)
{
  return &m_Vga;
}
size_t ArmBeagle::getNumVga()
{
  return 1;
}
IrqManager *ArmBeagle::getIrqManager()
{
  // TODO
  return 0;
}
SchedulerTimer *ArmBeagle::getSchedulerTimer()
{
  // TODO
  return 0;
}
Timer *ArmBeagle::getTimer()
{
  // TODO
  return 0;
}
Keyboard *ArmBeagle::getKeyboard()
{
  return &m_Keyboard;
}

ArmBeagle::ArmBeagle()
{
}
ArmBeagle::~ArmBeagle()
{
}


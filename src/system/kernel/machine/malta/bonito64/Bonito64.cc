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
#include "Bonito64.h"

Bonito64 Bonito64::m_Instance;

Machine &Machine::instance()
{
  return Bonito64::instance();
}

void Bonito64::initialise()
{
  m_Serial[0].setBase(0x1fd003f8);
  m_Serial[1].setBase(0x1fd002f8);
  m_bInitialised = true;
}
Serial *Bonito64::getSerial(size_t n)
{
  return &m_Serial[n];
}
size_t Bonito64::getNumSerial()
{
  return 2;
}
Vga *Bonito64::getVga(size_t n)
{
  return &m_Vga;
}
size_t Bonito64::getNumVga()
{
  return 1;
}
IrqManager *Bonito64::getIrqManager()
{
  // TODO
  return 0;
}
SchedulerTimer *Bonito64::getSchedulerTimer()
{
  // TODO return m_SchedulerTimer;
  return 0;
}
Timer *Bonito64::getTimer()
{
  // TODO return m_Timer;
  return 0;
}
Keyboard *Bonito64::getKeyboard()
{
  return &m_Keyboard;
}

Bonito64::Bonito64()
  : m_Vga()
{
}
Bonito64::~Bonito64()
{
}

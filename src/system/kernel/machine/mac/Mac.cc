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

#include "Mac.h"

Mac Mac::m_Instance;

Machine &Machine::instance()
{
  return Mac::instance();
}

void Mac::initialise()
{
  m_Vga.initialise();
  m_Keyboard.initialise();
  m_Decrementer.initialise();
  m_bInitialised = true;
}
Serial *Mac::getSerial(size_t n)
{
  return &m_Serial[n];
}
size_t Mac::getNumSerial()
{
  return 1;
}
Vga *Mac::getVga(size_t n)
{
  return &m_Vga;
}
size_t Mac::getNumVga()
{
  return 1;
}
IrqManager *Mac::getIrqManager()
{
  // TODO
  return 0;
}
SchedulerTimer *Mac::getSchedulerTimer()
{
  return &m_Decrementer;
}
Timer *Mac::getTimer()
{
  // TODO
  return 0;
}
Keyboard *Mac::getKeyboard()
{
  return &m_Keyboard;
}

Mac::Mac() :
  m_Decrementer(), m_Vga(), m_Keyboard()
{
}
Mac::~Mac()
{
}

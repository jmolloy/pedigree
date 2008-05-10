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

#include "Db1500.h"

Db1500 Db1500::m_Instance;

Machine &Machine::instance()
{
  return Db1500::instance();
}

void Db1500::initialise()
{
  // NOTE: Needs to set bInitialised to true
}
Serial *Db1500::getSerial(size_t n)
{
  return &m_Serial[n];
}
size_t Db1500::getNumSerial()
{
  return 2;
}
Vga *Db1500::getVga(size_t n)
{
  return &m_Vga;
}
size_t Db1500::getNumVga()
{
  return 1;
}
IrqManager *Db1500::getIrqManager()
{
  // TODO
  return 0;
}
SchedulerTimer *Db1500::getSchedulerTimer()
{
  // TODO return m_SchedulerTimer;
}
Timer *Db1500::getTimer()
{
  // TODO return m_Timer;
}

Db1500::Db1500()
  : m_Vga()
{
}
Db1500::~Db1500()
{
}

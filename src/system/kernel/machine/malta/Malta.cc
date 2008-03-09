/*
 * Copyright (c) 2008 James Molloy, James Pritchett, J�rg Pf�hler, Matthew Iselin
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
#include "Malta.h"

Malta Malta::m_Instance;

Machine &Machine::instance()
{
  return Malta::instance();
}

void Malta::initialise()
{
  // NOTE: Needs to set m_bInitialised to true
}
Serial *Malta::getSerial(size_t n)
{
  return &m_Serial[n];
}
size_t Malta::getNumSerial()
{
  return 2;
}
Vga *Malta::getVga(size_t n)
{
  return &m_Vga;
}
size_t Malta::getNumVga()
{
  return 1;
}
IrqManager *Malta::getIrqManager()
{
  // TODO
  return 0;
}
SchedulerTimer *Malta::getSchedulerTimer()
{
  // TODO
  return 0;
}
Timer *Malta::getTimer()
{
  // TODO
  return 0;
}

Malta::Malta()
{
}
Malta::~Malta()
{
}

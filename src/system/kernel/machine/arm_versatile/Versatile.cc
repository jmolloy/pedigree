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

#include "Versatile.h"

ArmVersatile ArmVersatile::m_Instance;

Machine &Machine::instance()
{
  return ArmVersatile::instance();
}

void ArmVersatile::initialise()
{
  // the versatile board QEMU emulates has two UARTs (PL011)
  // starting at 0x101f1000
  m_Serial[0].setBase(0x101f1000);
  //m_Serial[1].setBase(0x101f2000);
/// \todo setup second pl011 as well (removed to make it work) and detect existance of pl011s
  m_bInitialised = true;
}
Serial *ArmVersatile::getSerial(size_t n)
{
  return &m_Serial[n];
}
size_t ArmVersatile::getNumSerial()
{
  return 1;
}
Vga *ArmVersatile::getVga(size_t n)
{
  return &m_Vga;
}
size_t ArmVersatile::getNumVga()
{
  return 1;
}
IrqManager *ArmVersatile::getIrqManager()
{
  // TODO
  return 0;
}
SchedulerTimer *ArmVersatile::getSchedulerTimer()
{
  // TODO
  return 0;
}
Timer *ArmVersatile::getTimer()
{
  // TODO
  return 0;
}
Keyboard *ArmVersatile::getKeyboard()
{
  return &m_Keyboard;
}

ArmVersatile::ArmVersatile()
{
}
ArmVersatile::~ArmVersatile()
{
}

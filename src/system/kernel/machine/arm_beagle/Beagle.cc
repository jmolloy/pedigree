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
#include "Prcm.h"

ArmBeagle ArmBeagle::m_Instance;

Machine &Machine::instance()
{
  return ArmBeagle::instance();
}

void ArmBeagle::initialise()
{
  extern SyncTimer g_SyncTimer;

  Prcm::instance().initialise(0x48004000);

  m_Serial[0].setBase(0x49020000); // uart3, RS-232 output on board
  //m_Serial[1].setBase(0x4806A000); // uart1
  //m_Serial[2].setBase(0x4806C000); // uart2

  g_SyncTimer.initialise(0x48320000);

  m_Timers[0].initialise(0, 0x48318000);
  m_Timers[1].initialise(1, 0x49032000);
  m_Timers[2].initialise(2, 0x49034000);
  m_Timers[3].initialise(3, 0x49036000);
  m_Timers[4].initialise(4, 0x49038000);
  m_Timers[5].initialise(5, 0x4903A000);
  m_Timers[6].initialise(6, 0x4903C000);
  m_Timers[7].initialise(7, 0x4903E000);
  m_Timers[8].initialise(8, 0x49040000);
  m_Timers[9].initialise(9, 0x48086000);
  m_Timers[10].initialise(10, 0x48088000);

  m_bInitialised = true;
}
Serial *ArmBeagle::getSerial(size_t n)
{
  return &m_Serial[n];
}
size_t ArmBeagle::getNumSerial()
{
  return 1; // 3 UARTs attached, only one initialised for now
}
Vga *ArmBeagle::getVga(size_t n)
{
  return 0; // &m_Vga;
}
size_t ArmBeagle::getNumVga()
{
  return 0;
}
IrqManager *ArmBeagle::getIrqManager()
{
  // TODO
  return 0;
}
SchedulerTimer *ArmBeagle::getSchedulerTimer()
{
  return &m_Timers[1];
}
Timer *ArmBeagle::getTimer()
{
  return &m_Timers[1];
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


/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#include "Machine.h"

#include <Log.h>
#include <panic.h>

#include <machine/Device.h>
#include <machine/Bus.h>
#include <machine/Disk.h>
#include <machine/Controller.h>

HostedMachine HostedMachine::m_Instance;

Machine &Machine::instance()
{
  return HostedMachine::instance();
}

void HostedMachine::initialise()
{
  m_Serial[0].setBase(0);
  m_Vga.initialise();
  m_bInitialised = true;
}

void HostedMachine::initialiseDeviceTree()
{
}

Serial *HostedMachine::getSerial(size_t n)
{
  return &m_Serial[0];
}

size_t HostedMachine::getNumSerial()
{
  return 1;
}

Vga *HostedMachine::getVga(size_t n)
{
  return &m_Vga;
}

size_t HostedMachine::getNumVga()
{
  return 1;
}

IrqManager *HostedMachine::getIrqManager()
{
  return 0;
}

SchedulerTimer *HostedMachine::getSchedulerTimer()
{
  return 0;
}

Timer *HostedMachine::getTimer()
{
  return 0;
}

Keyboard *HostedMachine::getKeyboard()
{
  return 0;
}

void HostedMachine::setKeyboard(Keyboard *kb)
{
}

HostedMachine::HostedMachine()
{
}

HostedMachine::~HostedMachine()
{
}

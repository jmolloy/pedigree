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

#include "Serial.h"
#include <Log.h>

X86Serial::X86Serial()
  : m_Port("COM")
{
}

X86Serial::~X86Serial()
{
}

void X86Serial::setBase(uintptr_t nBaseAddr)
{
  m_Port.allocate(nBaseAddr, 8);

  m_Port.write8(0x00, serial::inten); // Disable all interrupts
  m_Port.write8(0x80, serial::lctrl); // Enable DLAB (set baud rate divisor)
  m_Port.write8(0x03, serial::rxtx);  // Set divisor to 3 (lo byte) 38400 baud
  m_Port.write8(0x00, serial::inten); //                  (hi byte)
  m_Port.write8(0x03, serial::lctrl); // 8 bits, no parity, one stop bit
  m_Port.write8(0xC7, serial::iififo);// Enable FIFO, clear them, with 14-byte threshold
  m_Port.write8(0x0B, serial::mctrl); // IRQs enabled, RTS/DSR set
  m_Port.write8(0x0C, serial::inten); // enable all interrupts.
  
  NOTICE("Modem status: " << Hex << m_Port.read8(serial::mstat));
  NOTICE("Line status: " << Hex << m_Port.read8(serial::lstat));
}

char X86Serial::read()
{
  if (!isConnected())
    return 0;
  while ( !(m_Port.read8(serial::lstat) & 0x1) ) ;
  
  return m_Port.read8(serial::rxtx);
}

char X86Serial::readNonBlock()
{
  if (!isConnected())
    return 0;
  if ( m_Port.read8(serial::lstat) & 0x1)
    return m_Port.read8(serial::rxtx);
  else
    return '\0';
}

void X86Serial::write(char c)
{
  if (!isConnected())
    return;
  while ( !(m_Port.read8(serial::lstat) & 0x20) ) ;
  
  m_Port.write8(static_cast<unsigned char> (c), serial::rxtx);
}

bool X86Serial::isConnected()
{
  return true;
  uint8_t nStatus = m_Port.read8(serial::mstat);
  // Bits 0x30 = Clear to send & Data set ready.
  // Mstat seems to be 0xFF when the device isn't present.
  if ((nStatus & 0x30) && nStatus != 0xFF)
    return true;
  else
    return false;
}

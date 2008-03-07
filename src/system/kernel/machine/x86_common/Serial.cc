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
#include "Serial.h"
#include <processor/io.h>

X86Serial::X86Serial() :
  m_nBaseAddr(0)
{
}

X86Serial::~X86Serial()
{
}

void X86Serial::setBase(uintptr_t nBaseAddr)
{
  m_nBaseAddr = static_cast<uint16_t> (nBaseAddr);
}

char X86Serial::read()
{
  IoPort port;
  port.allocate(m_nBaseAddr, 8);
  while ( !(port.read8(serial::lstat) & 0x1) ) ;
  
  return port.read8(serial::rxtx);
}

void X86Serial::write(char c)
{
  IoPort port;
  port.allocate(m_nBaseAddr, 8);
  while ( !(port.read8(serial::lstat) & 0x20) ) ;
  
  port.write8(static_cast<unsigned char> (c), serial::rxtx);
}

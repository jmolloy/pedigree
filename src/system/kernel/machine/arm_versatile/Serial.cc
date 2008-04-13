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

ArmVersatileSerial::ArmVersatileSerial() :
  m_pRegs(0)
{
}
ArmVersatileSerial::~ArmVersatileSerial()
{
}
void ArmVersatileSerial::setBase(uintptr_t nBaseAddr)
{
  m_pRegs = reinterpret_cast<serial*> (nBaseAddr);
}
char ArmVersatileSerial::read()
{
  uint32_t c = 0;
  while ( c == 0 ) c = m_pRegs->dr;
  return static_cast<char>(c);
}
char ArmVersatileSerial::readNonBlock()
{
  return static_cast<char>(m_pRegs->dr);
}
void ArmVersatileSerial::write(char c)
{
  m_pRegs->dr = static_cast<uint32_t>(c);
}

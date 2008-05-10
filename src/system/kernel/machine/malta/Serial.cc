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

#include <machine/malta/Serial.h>
#include <machine/types.h>
#include <Log.h>
#include <machine/Machine.h>
#include <utilities/StaticString.h>
MaltaSerial::MaltaSerial() :
  m_pBuffer(0),
  m_pRegs(0)
{
}

MaltaSerial::~MaltaSerial()
{
}

void MaltaSerial::setBase(uintptr_t nBaseAddr)
{
  // We use KSEG1 (uncached physical) for our IO accesses.
  m_pRegs = reinterpret_cast<serial*> (KSEG1(nBaseAddr));
}

void MaltaSerial::write(char c)
{
  while (!(m_pRegs->lstat & 0x20)) ;
  m_pRegs->rxtx = static_cast<uint8_t> (c);
}

char MaltaSerial::read()
{
  if (!isConnected())
    return 0;
  while (!(m_pRegs->lstat & 0x1)) ;
  return static_cast<char> (m_pRegs->rxtx);
}

char MaltaSerial::readNonBlock()
{
  if (!isConnected())
    return 0;
  if ( m_pRegs->lstat & 0x1)
    return m_pRegs->rxtx;
  else
    return '\0';
}

bool MaltaSerial::isConnected()
{
  return true;
  uint8_t nStatus = m_pRegs->mstat;
  // Bits 0x30 = Clear to send & Data set ready.
  // Mstat seems to be 0xFF when the device isn't present.
  if ((nStatus & 0x30) && nStatus != 0xFF)
    return true;
  else
    return false;
}

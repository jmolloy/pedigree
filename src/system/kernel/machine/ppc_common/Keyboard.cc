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

#include "Keyboard.h"

PPCKeyboard::PPCKeyboard() :
  m_Dev(0)
{
}

PPCKeyboard::~PPCKeyboard()
{
}

void PPCKeyboard::initialise()
{
//   m_Dev = OFDevice(OpenFirmware::instance().findDevice("/keyboard"));
  OFDevice chosen(OpenFirmware::instance().findDevice("/chosen"));
  m_Dev = chosen.getProperty("stdin");
//   for(;;);
}

char PPCKeyboard::getChar()
{
  char c[4];
  while (OpenFirmware::instance().call("read", 3, m_Dev,
                                       reinterpret_cast<OFParam> (c),
                                       reinterpret_cast<OFParam> (4)) != 0)
    ;
  if ((c[0] < 'A' || c[0] > 'z') && c[0] != '\r' && c[0] != 0x08 && c[0] != 0x09)
    return 0;
  else
    return c[0];
}

char PPCKeyboard::getCharNonBlock()
{
  char c[4];
  if (OpenFirmware::instance().call("read", 3, m_Dev,
                                    reinterpret_cast<OFParam> (c),
                                    reinterpret_cast<OFParam> (4)) != 0)
    return 0;

  if (c[0] == '\r')
    return '\n';
  else
    return c[0];
}


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

#include "Vt100.h"
#include "../../kernel/core/BootIO.h"
#include <machine/Machine.h>
#include <machine/Keyboard.h>

Vt100::Vt100() :
  bufLen(0), bufStart(0), bufEnd(0)
{
}

Vt100:~Vt100()
{
}

void Vt100::write(char c)
{
}

char Vt100::read()
{
  if (bufLen > 0)
  {
    return unbufferChar();
  }
  else
  {
    return Machine::instance().getKeyboard()->getCharNonBlock();
  }
}

void Vt100::bufferChar(char c)
{
  // TODO needs a lock.
  buffer[(bufEnd++)%MAXBUFLEN] = c;
  bufLen.release(1);
}

char Vt100::unbufferChar()
{
  bufLen.acquire();
  return buffer[(bufStart++)%MAXBUFLEN];
}

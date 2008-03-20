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
#ifndef MACHINE_X86_SERIAL_H
#define MACHINE_X86_SERIAL_H

#include <compiler.h>
#include <processor/types.h>
#include <processor/IoPort.h>
#include <machine/Serial.h>

namespace serial
{
  const int rxtx    =0;
  const int inten   =1;
  const int iififo  =2;
  const int lctrl   =3;
  const int mctrl   =4;
  const int lstat   =5;
  const int mstat   =6;
  const int scratch =7;
}

/**
 * Serial device abstraction.
 */
class X86Serial : public Serial
{
  public:
    X86Serial();
    virtual void setBase(uintptr_t nBaseAddr);
    virtual ~X86Serial();
  
    virtual char read();
    virtual char readNonBlock();
    virtual void write(char c);
  private:
    IoPort m_Port;
};

#endif

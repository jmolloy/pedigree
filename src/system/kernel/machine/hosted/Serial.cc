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

#include "Serial.h"
#include <utilities/StaticString.h>

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

HostedSerial::HostedSerial() : m_File(-1), m_nFileNumber(0)
{
}

HostedSerial::~HostedSerial()
{
    if(m_File >= 0)
    {
        close(m_File);
        m_File = -1;
    }
}

void HostedSerial::setBase(uintptr_t nBaseAddr)
{
    m_nFileNumber = nBaseAddr;
    if(m_File >= 0)
        close(m_File);

    NormalStaticString s;
    s.clear();
    s.append("serial");
    s.append(m_nFileNumber);
    s.append(".log");
    m_File = open(static_cast<const char *>(s), O_TRUNC | O_CREAT | O_WRONLY, 0644);
}

char HostedSerial::read()
{
  // Cannot do.
  return '\0';
}

char HostedSerial::readNonBlock()
{
  return read();
}

void HostedSerial::write(char c)
{
  char buf[2] = {c, 0};
  ::write(m_File, buf, 1);
}

bool HostedSerial::isConnected()
{
  return m_File >= 0;
}

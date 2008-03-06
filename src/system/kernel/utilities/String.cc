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
#include <utilities/utility.h>
#include <utilities/String.h>

String::String(const char *s)
  : m_Data(0), m_Length(strlen(s)), m_Size(0)
{
  assign(s);
}
String::String(const String &x)
  : m_Data(0), m_Length(x.length()), m_Size(0)
{
  assign(x);
}
String::~String()
{
  delete []m_Data;
}

String &String::operator = (const String &x)
{
  assign(x);
  return *this;
}
String &String::operator = (const char *s)
{
  assign(s);
  return *this;
}

void String::assign(const String &x)
{
  reserve(x.length() + 1);
  memcpy(m_Data, x.m_Data, x.length() + 1);
  m_Length = x.length() + 1;
}
void String::assign(const char *s)
{
  m_Length = strlen(s);
  reserve(m_Length + 1);
  memcpy(m_Data, s, m_Length + 1);
}
void String::reserve(size_t size)
{
  if (size > m_Size)
  {
    char *tmp = m_Data;
    m_Data = new char [size];
    if (tmp != 0 && m_Size != 0)
      memcpy(m_Data, tmp, m_Size);
    delete []tmp;
    m_Size = size;
  }
}

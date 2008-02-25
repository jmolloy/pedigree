/*
 * Copyright (c) 2008 James Molloy, James Pritchett, J�rg Pf�hler, Matthew Iselin
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
#include <StringBase.h>
#include <Log.h>

StringBase::StringBase() :
  m_pData(0),
  m_nBufLen(0)
{
}

StringBase &StringBase::operator+=(StringBase rhs)
{
  append(rhs);
  return *this;
}

StringBase &StringBase::operator+=(uintptr_t rhs)
{
  append(rhs);
  return *this;
}

StringBase &StringBase::operator=(const StringBase &rhs)
{
  resize(rhs.length()+1);
  strcpy(m_pData, rhs.m_pData);
  m_pData[rhs.length()] = '\0';
  return *this;
}

int StringBase::operator[](const char c)
{
  for(unsigned int i = 0; i < length(); i++)
    if (m_pData[i] == c)
      return i;
  return -1;
}

char StringBase::operator[](int n)
{
  if (n > 0)
    return m_pData[n];
  else
    return m_pData[strlen(m_pData)-n];
}

StringBase::operator const char *()
{
  return static_cast<const char*> (m_pData);
}

size_t StringBase::length() const
{
  return strlen(m_pData);
}

void StringBase::append(StringBase pStr, size_t nLen, char c)
{
}

void StringBase::append(uintptr_t nInt, int nRadix, size_t nLen, char c)
{
}

size_t StringBase::resize(size_t nSz)
{
  FATAL("Pure virtual StringBase::resize() called!");
  asm volatile("int $3");
  return 0;
}

StringBase StringBase::left(int n)
{
  FATAL("Pure virtual StringBase::left() called!");
  StringBase str;
  return str;
}

StringBase StringBase::right(int n)
{
  FATAL("Pure virtual StringBase::right() called!");
  StringBase str;
  return str;
}

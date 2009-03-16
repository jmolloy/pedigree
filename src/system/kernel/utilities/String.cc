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

#include <utilities/utility.h>
#include <utilities/String.h>
#include <Log.h>

void String::assign(const String &x)
{
  m_Length = x.length();
  reserve(m_Length + 1);
  if (m_Length)
    memcpy(m_Data, x.m_Data, m_Length + 1);
}
void String::assign(const char *s)
{
  m_Length = strlen(s);
  reserve(m_Length + 1);
  if (m_Length)
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
void String::free()
{
  delete [] m_Data;
  m_Data = 0;
  m_Length = 0;
  m_Size = 0;
}

String String::split(size_t offset)
{
  String s(&m_Data[offset]);
  m_Data[offset] = '\0';
  m_Length = offset;

  return s;
}

List<String*> String::tokenise(char token)
{
  String copy = *this;
  List<String*> list;

  size_t idx = 0;
  while (idx < copy.m_Length)
  {
    if (copy.m_Data[idx] == token)
    {
      String tmp = copy.split(idx+1);

      String *pStr = new String(copy);
      copy = tmp;

      // pStr will include token, so remove the last character from it.
      pStr->m_Length --;
      pStr->m_Data[pStr->length()] = '\0';

      list.pushBack(pStr);
      idx = 0;
    }
    else
      idx = copy.nextCharacter(idx);
  }

  list.pushBack(new String(copy));

  return list;
}

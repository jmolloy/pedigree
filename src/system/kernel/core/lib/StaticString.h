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
#ifndef STATICSTRING_H
#define STATICSTRING_H

#include <utility.h>
#include <Log.h>

/**
 * Derivative of StringBase that uses a statically allocated chunk of memory.
 */
template<unsigned int N>
class StaticString
{
public:
  /**
   * Default constructor.
   */
  StaticString()
  {
    m_pData[0] = '\0';
  }
  
  /**
   * Creates a StaticString from a const char *.
   * This creates a new copy of pSrc - pSrc can be safely
   * deallocated afterwards.
   */
  StaticString(const char *pSrc)
  {
    strncpy(m_pData, pSrc, N);
    m_pData[strlen(pSrc)] = '\0';
  }
  
  /**
   * Copy constructor - creates a StaticString from another StaticString.
   * Copies the memory associated with src.
   */
  StaticString(const StaticString &src)
  {
    strncpy(m_pData, src.m_pData, N);
    m_pData[src.length()] = '\0';
  }
  
  /**
   * Virtual destructor.
   */
  virtual ~StaticString()
  {
  }
  
  operator const char*()
  {
    return (const char*)m_pData;
  }
  
  StaticString &operator+=(const StaticString &str)
  {
    append(str);
//      NOTICE("My string: " << *this);
    return *this;
  }
  
  StaticString left(int n)
  {
    StaticString<N> str;
    strncpy(str.m_pData, m_pData, n);
    str.m_pData[n] = '\0';
    return str;
  }

  StaticString right(int n)
  {
    StaticString<N> str;
    strncpy(str.m_pData, &m_pData[strlen(m_pData)-n], n);
    str.m_pData[n] = '\0';
    return str;
  }

  void append(uintptr_t nInt, int nRadix=10, size_t nLen=0, char c=' ')
  {
    char pStr[64];
    char pFormat[8];
    const char *pRadix;
    switch (nRadix)
    {
      case 8:
        pRadix = "o";
        break;
      case 10:
        pRadix = "d";
        break;
      case 16:
        pRadix = "x";
        break;
    }
  
    sprintf(pFormat, "%%%s", pRadix);
  
    sprintf(pStr, pFormat, nInt);
    append(pStr, nLen, c);
  }
  
  void append(StaticString pStr, size_t nLen=0, char c=' ')
  {
    if (nLen < pStr.length())
    {
      strcat(m_pData, pStr.m_pData);
      NOTICE("append: here." << m_pData);
    }
    else
    {
      unsigned int i;
      for(i = length(); i < nLen - pStr.length() + length(); i++)
      {
        m_pData[i] = c;
      }
      m_pData[i] = '\0';
      strcat(m_pData, pStr.m_pData);
    }
  }
  
  size_t length() const
  {
    return N;
  }
  
private:
  /**
   * Our actual static data.
   */
  char m_pData[N];
};

#endif

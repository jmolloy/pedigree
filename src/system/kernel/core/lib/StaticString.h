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
    if (static_cast<unsigned int>(strlen(pSrc)) >= N)
      m_pData[N-1] = '\0';
    else
      m_pData[strlen(pSrc)] = '\0';
  }
  
  /**
   * Copy constructor - creates a StaticString from another StaticString.
   * Copies the memory associated with src.
   */
  StaticString(const StaticString &src)
  {
    strncpy(m_pData, src.m_pData, N);
    if (src.length() >= N)
      m_pData[N-1] = '\0';
    else
      m_pData[src.length()] = '\0';
  }
  
  /**
   * Virtual destructor.
   */
  virtual ~StaticString()
  {
  }
  
  operator const char*() const 
  {
    return static_cast<const char*>(m_pData);
  }
  
  StaticString &operator+=(const StaticString &str)
  {
    append(str);
    return *this;
  }
  
  StaticString &operator+=(const int &i)
  {
    append(i);
    return *this;
  }
  
  StaticString &operator+=(const char &i)
  {
    char ch[2];
    ch[0] = i;
    ch[1] = '\0';
    append(ch);
    return *this;
  }

  bool operator==(const char* pStr) const
  {
    return (strcmp(m_pData, pStr) == 0);
  }

  bool operator==(StaticString &other) const
  {
    return (strcmp(m_pData, other.m_pData) == 0);
  }

  int last(const char search)
  {
    for (int i = length(); i >= 0; i--)
      if (m_pData[i] == search)
        return i;
    return -1;
  }
  
  bool contains(const char *other) const
  {
    if (strlen(other) >= length())
      return (strcmp(m_pData, other) == 0);
    
    int mylen = length();
    int itslen = strlen(other);
    for (int i = 0; i < mylen-itslen+1; i++)
      if (strcmp(&m_pData[i], other) == 0)
        return true;
    return false;
  }
    
  bool contains(StaticString &other) const
  {
    if (other.length() >= length())
      return (strcmp(m_pData, other.m_pData) == 0);
    
    int mylen = length();
    int itslen = other.length();
    for (int i = 0; i < mylen-itslen+1; i++)
      if (strcmp(&m_pData[i], other.m_pData) == 0)
        return true;
    return false;
  }
  
  int intValue(int nBase=0) const
  {
    return strtoul(m_pData, 0, nBase);
  }
  
  StaticString left(int n) const
  {
    StaticString<N> str;
    strncpy(str.m_pData, m_pData, n);
    str.m_pData[n] = '\0';
    return str;
  }

  StaticString right(int n) const
  {
    StaticString<N> str;
    strncpy(str.m_pData, &m_pData[strlen(m_pData)-n], n);
    str.m_pData[n] = '\0';
    return str;
  }

  StaticString &stripFirst(int n=1)
  {
    if (n > strlen(m_pData))
      return *this;
    int i;
    for (i = n; m_pData[i] != '\0'; i++)
      m_pData[i-n] = m_pData[i];
    m_pData[i-n] = '\0';
    return *this;
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

  void append(const StaticString &str, size_t nLen=0, char c=' ')
  {
    if (nLen < str.length())
    {
      strncat(m_pData, str.m_pData, N-strlen(m_pData));
      // Ensure our last character is '\0', so strlen's can't fuck up.
      m_pData[N-1] = '\0';
    }
    else
    {
      unsigned int i;
      size_t len = length();
      for(i = len; i < nLen - str.length() + len; i++)
      {
        m_pData[i] = c;
      }
      m_pData[i] = '\0';
      strcat(m_pData, str.m_pData);
    }
  }

  size_t length() const
  {
    return strlen(m_pData);
  }
  
  private:
  /**
   * Our actual static data.
   */
  char m_pData[N];
};

typedef StaticString<32>   TinyStaticString;
typedef StaticString<64>   NormalStaticString;
typedef StaticString<128>  LargeStaticString;
typedef StaticString<1024> HugeStaticString;

#endif

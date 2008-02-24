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
#ifndef STATICSTRING_H
#define STATICSTRING_H

#include <StringBase.h>
#include <Log.h>

/**
 * Derivative of StringBase that uses a statically allocated chunk of memory.
 */
template<unsigned int N>
class StaticString : public StringBase
{
public:
  /**
   * Default constructor.
   */
  StaticString()
  {
    m_pData = static_cast<char*> (m_pStaticData);
    m_nBufLen = N;
    m_pData[0] = '\0';
  }
  
  /**
   * Creates a StaticString from a const char *.
   * This creates a new copy of pSrc - pSrc can be safely
   * deallocated afterwards.
   */
  StaticString(const char *pSrc)
  {
    m_pData = static_cast<char*> (m_pStaticData);
    m_nBufLen = N;
    strcpy(m_pData, pSrc);
    m_pData[strlen(pSrc)] = '\0';
  }
  
  /**
   * Copy constructor - creates a StaticString from another StaticString.
   * Copies the memory associated with src.
   */
  StaticString(StaticString &src)
  {
    m_pData = static_cast<char*> (m_pStaticData);
    m_nBufLen = N;
    strcpy(m_pData, src.m_pData);
    m_pData[src.length()] = '\0';
  }
  
  /**
   * Virtual destructor.
   */
  virtual ~StaticString()
  {
  }
  
  virtual StringBase left(int n)
  {
    StaticString<N> str;
    if (str.resize(n+1) != static_cast<unsigned int>(n+1))
      return str;
    strncpy(str.m_pData, m_pData, n);
    str.m_pData[n] = '\0';
    return str;
  }

  virtual StringBase right(int n)
  {
    StaticString<N> str;
    if (str.resize(n+1) != static_cast<unsigned int>(n+1))
      return str;
    strncpy(str.m_pData, &m_pData[strlen(m_pData)-n], n);
    str.m_pData[n] = '\0';
    return str;
  }

protected:
  /**
   * Called when the StringBase wants to resize its buffer.
   * \param[in] nSz The requested total buffer size.
   * \return The new buffer size.
   */
  virtual size_t resize(size_t nSz)
  {
    NOTICE("StaticString resize called.");
    if (nSz > N)
      ERROR("StaticString size exceeded - " << Dec << nSz << " > " << N);
    m_pData = reinterpret_cast<char*>(&m_pStaticData);
    return N;
  }
  
private:
  /**
   * Our actual static data.
   */
  char m_pStaticData[N];
};

#endif

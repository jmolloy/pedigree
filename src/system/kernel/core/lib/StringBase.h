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
#ifndef STRINGBASE_H
#define STRINGBASE_H

#include <processor/types.h>
#include <utility.h>

/**
 * Class to handle string manipulation and formatting. This is a base class
 * that ignores everything memory-management related. That is implemented
 * in the derived classes, StaticString and DynamicString.
 */
class StringBase
{
public:
  /**
   * Default constructor.
   */
  StringBase();
  
  /**
   * Creates a StringBase from a const char *.
   * This creates a new copy of pSrc - pSrc can be safely
   * deallocated afterwards.
   */
  StringBase(const char *pSrc);
  
  /**
   * Copy constructor - creates a StringBase from another StringBase.
   * Copies the memory associated with src.
   */
  StringBase(StringBase &src);
  
  /**
   * Virtual destructor.
   */
  virtual ~StringBase() {}
  
  /**
   * Operator overloads for string concetenation.
   */
  StringBase &operator+=(StringBase &rhs);
  StringBase &operator+=(uintptr_t rhs);
  
  StringBase &operator=(const StringBase &rhs);
  operator const char *();
  
  /**
   * Returns the first occurrence of c in the string.
   */
  int operator[](const char c);
  /**
   * Returns the character at location n, from the front of the string if n is positive
   * and the end of the string (not including the NULL terminator) if n is negative.
   */
  char operator[](int n);
  
  /** Returns the length of the string, not including the NULL terminator. */
  size_t length() const;
  /** Returns a StringBase containing only the first n characters from this string. */
  virtual StringBase left(int n);
  /** Returns a StringBase containing only the last n characters from this string. */
  virtual StringBase right(int n);
  
  /** Appends a string to this string, padding to nLen with c, if required. */
  void append(StringBase pStr, size_t nLen=0, char c=' ');
  /** Appends an integer to this string, in the given radix, padding to nLen with c, if required.
   * \note Currently, radix only supports values of 10, 16 and 8. */
  void append(uintptr_t nInt, int nRadix=10, size_t nLen=0, char c=' ');
  
protected:
  /**
   * Called when the StringBase wants to resize its buffer.
   * \param[in] nSz The requested total buffer size.
   * \return The new buffer size.
   */
  virtual size_t resize(size_t nSz);
  
  /**
   * Pointer to our member C-style string.
   */
  char *m_pData;
  
  /**
   * Length of the buffer that m_pData inhabits.
   */
  size_t m_nBufLen;
  
};

#endif


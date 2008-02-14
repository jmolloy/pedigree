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
#ifndef STACKFRAME_H
#define STACKFRAME_H

#include <utility.h>

class StackFrame
{
public:
  /**
   * Creates a stack frame based on the given Base Pointer value, and also the given
   * symbol name (mangled).
   */
  StackFrame(unsigned int nBasePointer, const char *pMangledSymbol);
  ~StackFrame();

  /**
   * Returns a pretty printed string containing the function name and each parameter with
   * its value (hopefully).
   */
  void prettyPrint(char *pBuf, unsigned int nBufLen);
  
private:
  /**
   * Returns the n'th 32/64-bit parameter in the stack frame.
   */
  unsigned int getParameter(unsigned int n);
  
  /**
   * Formats a number, given the 'type' of that number.
   */
  void format(unsigned int n, const char *pType, char *pDest);
  
  unsigned int m_nBasePointer;
  symbol_t m_Symbol;
};

#endif

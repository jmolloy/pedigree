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

#include <utilities/utility.h>
#include <demangle.h>
#include <processor/types.h>
#include <utilities/StaticString.h>

class StackFrame
{
public:
  /**
   * Creates a stack frame based on the given Base Pointer value, and also the given
   * symbol name (mangled).
   */
  StackFrame(unsigned int nBasePointer, LargeStaticString mangledSymbol);
  ~StackFrame();

  /**
   * Returns a pretty printed string containing the function name and each parameter with
   * its value (hopefully).
   */
  void prettyPrint(HugeStaticString &buf);
  
private:
  /**
   * Returns the n'th 32/64-bit parameter in the stack frame.
   */
  unsigned int getParameter(unsigned int n);
  
  /**
   * Returns whether the symbol is a class member function.
   * This is calculated by a simple algorithm:
   *  * If the symbol name contains no '::'s, return false.
   *  * If the letter directly after the last '::' is a capital, return true, else return false.
   */
  bool isClassMember();
  
  /**
   * Formats a number, given the 'type' of that number.
   */
  void format(unsigned int n, const LargeStaticString &type, HugeStaticString &dest);
  
  uintptr_t m_nBasePointer;
  symbol_t m_Symbol;
};

#endif

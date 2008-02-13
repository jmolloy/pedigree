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

#ifndef BACKTRACE_H
#define BACKTRACE_H

#define MAX_STACK_FRAMES 20

class Backtrace
{
public:
  /**
   * Creates a new Backtrace object.
   */
  Backtrace();
  ~Backtrace();
  
  /**
   * Performs a backtrace from the given base pointer address, or if none was specified,
   * the current EBP location.
   */
  void performBacktrace(unsigned int address=0);
  
  /**
   * Returns the number of stack frames retrieved.
   */
  unsigned int numStackFrames();
  
  /**
   * Returns the return address of the n'th stack frame.
   */
  unsigned int getReturnAddress(unsigned int n);
  
  /**
   * Returns the base pointer of the n'th stack frame.
   */
  unsigned int getBasePointer(unsigned int n);
  
  void prettyPrint(char *pBuffer, unsigned int nBufferLength);
  
private:
  /**
   * The return addresses.
   */
  unsigned int m_pReturnAddresses[MAX_STACK_FRAMES];
  /**
   * The base pointers.
   */
  unsigned int m_pBasePointers[MAX_STACK_FRAMES];
  /**
   * The number of stack frames retrieved by a performBacktrace call.
   */
  unsigned int m_nStackFrames;
};

#endif

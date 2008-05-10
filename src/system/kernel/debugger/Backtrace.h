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

#ifndef BACKTRACE_H
#define BACKTRACE_H

#include <processor/Processor.h>
#include <processor/StackFrame.h>
#include <utilities/StaticString.h>

/** @addtogroup kerneldebugger
 * @{ */

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
  void performBacktrace(InterruptState &state);
  
  /**
   * Returns the number of stack frames retrieved.
   */
  size_t numStackFrames();
  
  /**
   * Returns the return address of the n'th stack frame.
   */
  uintptr_t getReturnAddress(size_t n);
  
  /**
   * Returns the base pointer of the n'th stack frame.
   */
  uintptr_t getBasePointer(size_t n);
  
  void prettyPrint(HugeStaticString &buf, size_t nFrames=0, size_t nFromFrame=0);
  
private:
  /**
   * Performs a DWARF backtrace.
   */
  void performDwarfBacktrace(InterruptState &state);
  
  /**
   * Performs a "normal" backtrace, based on following a linked list of frame pointers.
   */
  void performBpBacktrace(uintptr_t base, uintptr_t instruction);
  
  /**
   * The return addresses.
   */
  uintptr_t m_pReturnAddresses[MAX_STACK_FRAMES];
  /**
   * The base pointers.
   */
  uintptr_t m_pBasePointers[MAX_STACK_FRAMES];
  
  /**
   * Processor register states.
   */
  ProcessorState m_pStates[MAX_STACK_FRAMES];
  
  /**
   * The number of stack frames retrieved by a performBacktrace call.
   */
  size_t m_nStackFrames;
};

/** @} */

#endif

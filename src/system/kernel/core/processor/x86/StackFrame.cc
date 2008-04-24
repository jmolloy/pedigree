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
#ifdef DEBUGGER

#include <processor/StackFrame.h>
#include <stdarg.h>
#include <Log.h>

uintptr_t X86StackFrame::getParameter(size_t n)
{
  #if defined(OMIT_FRAMEPOINTER)
    uint32_t *pPtr = reinterpret_cast<uint32_t*>(m_BasePointer + (n - 1) * sizeof(uint32_t));
  #else
    uint32_t *pPtr = reinterpret_cast<uint32_t*>(m_BasePointer + n * sizeof(uint32_t));
  #endif
  return *pPtr;
}

void X86StackFrame::construct(ProcessorState &state,
                              uintptr_t returnAddress,
                              unsigned int nParams,
                              ...)
{
  // Obtain the stack pointer.
  uintptr_t *pStack = reinterpret_cast<uintptr_t*> (state.getStackPointer());
  
  // How many parameters do we need to push?
  // We push in reverse order but must iterate through the va_list in forward order,
  // so we decrement the stack pointer here.
  pStack -= nParams+1; // +1 for return address.
  uintptr_t *pStackLowWaterMark = pStack;
  
  *++pStack = returnAddress;
  
  va_list list;
  va_start(list, nParams);
  
  for(int i = nParams-1; i >= 0; i--)
  {
    *++pStack = va_arg(list, uintptr_t);
  }
  
  va_end(list);
  
  // Write the new stack pointer back.
  state.setStackPointer(reinterpret_cast<uintptr_t> (pStackLowWaterMark));
}

#endif

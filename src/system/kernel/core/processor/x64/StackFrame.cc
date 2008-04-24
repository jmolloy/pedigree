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
#if defined(DEBUGGER)

#include <processor/StackFrame.h>

uintptr_t X64StackFrame::getParameter(size_t n)
{
  if (n == 0)return m_State.rdi;
  if (n == 1)return m_State.rsi;
  if (n == 2)return m_State.rdx;
  if (n == 3)return m_State.rcx;
  if (n == 4)return m_State.r8;
  if (n == 5)return m_State.r9;

  #if defined(OMIT_FRAMEPOINTER)
    uint64_t *pPtr = reinterpret_cast<uint64_t*>(m_State.rbp + (n - 1) * sizeof(uint64_t));
  #else
    uint64_t *pPtr = reinterpret_cast<uint64_t*>(m_State.rbp + n * sizeof(uint64_t));
  #endif
  return *pPtr;
}

void X64StackFrame::construct(ProcessorState &state,
                              uintptr_t returnAddress,
                              unsigned int nParams,
                              ...)
{
  // TODO
}

#endif

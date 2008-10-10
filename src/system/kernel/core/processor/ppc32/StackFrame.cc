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

#include <processor/StackFrame.h>
#include <Log.h>

#if defined(DEBUGGER)

  uintptr_t PPC32StackFrame::getParameter(size_t n)
  {
    if (n == 0)return m_State.m_R3;
    if (n == 1)return m_State.m_R4;
    if (n == 2)return m_State.m_R5;
    if (n == 3)return m_State.m_R6;
    if (n == 4)return m_State.m_R7;
    if (n == 5)return m_State.m_R8;
    if (n == 6)return m_State.m_R9;
    if (n == 7)return m_State.m_R10;
  
    // Slightly difficult - have to follow the back chain pointer then 
    // go up 2 words to skip the next back chain and the saved LR.
    WARNING("PPC32StackFrame: More than 8 parameters not implemented yet.");
//    uint64_t *pPtr = reinterpret_cast<uint64_t*>(m_State.m_R1 + (n - 8) * sizeof(uint64_t));
//    return *pPtr;
    return 0;
  }

#endif

void PPC32StackFrame::construct(ProcessorState &state,
                                uintptr_t returnAddress,
                                unsigned int nParams,
                                ...)
{
  state.m_Lr = returnAddress;
  
  va_list list;
  va_start(list, nParams);
  
  for(unsigned int i = 0; i < nParams; i++)
  {
    uintptr_t arg = va_arg(list, uintptr_t);
    NOTICE("Setting " << Hex << i << ", " << arg);
    switch (i)
    {
      case 0: state.m_R3 = arg; break;
      case 1: state.m_R4 = arg; break;
      case 2: state.m_R5 = arg; break;
      case 3: state.m_R6 = arg; break;
      case 4: state.m_R7 = arg; break;
      case 5: state.m_R8 = arg; break;
      default: ERROR("StackFrame: Too many parameters");
    }
  }
  
  va_end(list);
}

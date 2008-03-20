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
#include <DwarfUnwinder.h>
#include <DwarfState.h>

DwarfUnwinder::DwarfUnwinder(uintptr_t nData, size_t nLength) :
  m_nData(nData),
  m_nLength(nLength)
{
}

DwarfUnwinder::~DwarfUnwinder()
{
}

bool DwarfUnwinder::unwind(const InterruptState &inState, InterruptState &outState)
{
  // Construct a DwarfState object and populate it.
  DwarfState startState;
  
  // Unfortunately the next few lines are highly architecture dependent.
#ifdef X86
  startState.m_R[DWARF_REG_EAX] = inState.m_Eax;
  startState.m_R[DWARF_REG_EBX] = inState.m_Ebx;
  startState.m_R[DWARF_REG_ECX] = inState.m_Ecx;
  startState.m_R[DWARF_REG_EDX] = inState.m_Edx;
  startState.m_R[DWARF_REG_ESI] = inState.m_Esi;
  startState.m_R[DWARF_REG_EDI] = inState.m_Edi;
  startState.m_R[DWARF_REG_ESP] = inState.m_Esp;
  startState.m_R[DWARF_REG_EBP] = inState.m_Ebp;
#endif
  
  
  
  
}

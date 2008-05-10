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

#include "Disassembler.h"

X86Disassembler::X86Disassembler()
  : m_Location(0), m_Mode(32), m_Obj()
{
  ud_init(&m_Obj);
  ud_set_mode(&m_Obj, m_Mode);
  ud_set_syntax(&m_Obj, UD_SYN_INTEL);
  ud_set_pc(&m_Obj, m_Location);
  ud_set_input_buffer(&m_Obj, reinterpret_cast<uint8_t*>(m_Location), 4096);
}

X86Disassembler::~X86Disassembler()
{
}

void X86Disassembler::setLocation(uintptr_t nLocation)
{
  m_Location = nLocation;
  ud_set_mode(&m_Obj, m_Mode);
  ud_set_pc(&m_Obj, m_Location);
  ud_set_input_buffer(&m_Obj, reinterpret_cast<uint8_t*>(m_Location), 4096);
  ud_disassemble(&m_Obj);
}

uintptr_t X86Disassembler::getLocation()
{
  return m_Location;
}

void X86Disassembler::setMode(size_t nMode)
{
  if (nMode == 32 || nMode == 64)
    m_Mode = nMode;
  ud_set_mode(&m_Obj, m_Mode);
}

void X86Disassembler::disassemble(LargeStaticString &text)
{
  text += ud_insn_asm(&m_Obj);
  
  ud_disassemble(&m_Obj);
  m_Location = ud_insn_off(&m_Obj);
}

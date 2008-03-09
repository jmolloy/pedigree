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
#include <processor/state.h>

const char *MIPS32InterruptStateRegisterName[36] =
{
  "zero",
  "at",
  "v0",
  "v1",
  "a0",
  "a1",
  "a2",
  "a3",
  "t0",
  "t1",
  "t2",
  "t3",
  "t4",
  "t5",
  "t6",
  "t7",
  "s0",
  "s1",
  "s2",
  "s3",
  "s4",
  "s5",
  "s6",
  "s7",
  "t8",
  "t9",
  "k0",
  "k1",
  "gp",
  "sp",
  "fp",
  "ra",
  "status",
  "cause",
  "epc",
  "config"
};

size_t MIPS32InterruptState::getRegisterCount() const
{
  return 36;
}
processor_register_t MIPS32InterruptState::getRegister(size_t index) const
{
//   if (index == 0)return m_Eax;
//   if (index == 1)return m_Ebx;
//   if (index == 2)return m_Ecx;
//   if (index == 3)return m_Edx;
//   if (index == 4)return m_Edi;
//   if (index == 5)return m_Esi;
//   if (index == 6)return m_Ebp;
//   if (index == 7)return m_Esp;
//   if (index == 8)return m_Eip;
//   if (index == 9)return m_Eflags;
  return 0;
}
const char *MIPS32InterruptState::getRegisterName(size_t index) const
{
  return MIPS32InterruptStateRegisterName[index];
}

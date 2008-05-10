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

#include <processor/state.h>

const char *ARM926EInterruptStateRegisterName[17] =
{
  "r0",
  "r1",
  "r2",
  "r3",
  "r4",
  "r5",
  "r6",
  "r7",
  "r8",
  "r9",
  "r10",
  "bp",
  "r12",
  "sp",
  "r14",
  "pc",
  "cpsr"
};

ARM926EInterruptState::ARM926EInterruptState() :
    m_r0(), m_r1(), m_r2(), m_r3(),
    m_r4(), m_r5(), m_r6(), m_r7(),
    m_r8(), m_r9(), m_r10(), m_Fp(),
    m_r12(), m_Sp(), m_Lnk(), m_Pc(),
    m_Cpsr()
{
}

ARM926EInterruptState::ARM926EInterruptState(const ARM926EInterruptState &is) :
    m_r0(is.m_r0), m_r1(is.m_r1), m_r2(is.m_r2), m_r3(is.m_r3),
    m_r4(is.m_r4), m_r5(is.m_r5), m_r6(is.m_r6), m_r7(is.m_r7),
    m_r8(is.m_r8), m_r9(is.m_r9), m_r10(is.m_r10), m_Fp(is.m_Fp),
    m_r12(is.m_r12), m_Sp(is.m_Sp), m_Lnk(is.m_Lnk), m_Pc(is.m_Pc),
    m_Cpsr(is.m_Cpsr)
{
}

ARM926EInterruptState & ARM926EInterruptState::operator=(const ARM926EInterruptState &is)
{
  m_r0 = is.m_r0; m_r1 = is.m_r1; m_r2 = is.m_r2; m_r3 = is.m_r3;
  m_r4 = is.m_r4; m_r5 = is.m_r5; m_r6 = is.m_r6; m_r7 = is.m_r7;
  m_r8 = is.m_r8; m_r9 = is.m_r9; m_r10 = is.m_r10; m_Fp = is.m_Fp;
  m_r12 = is.m_r12; m_Sp = is.m_Sp; m_Lnk = is.m_Lnk; m_Pc = is.m_Pc;
  m_Cpsr = is.m_Cpsr;
  return *this;
}

size_t ARM926EInterruptState::getRegisterCount() const
{
  return 17;
}
processor_register_t ARM926EInterruptState::getRegister(size_t index) const
{
  switch (index)
  {
    case 0: return m_r0;
    case 1: return m_r1;
    case 2: return m_r2;
    case 3: return m_r3;
    case 4: return m_r4;
    case 5: return m_r5;
    case 6: return m_r6;
    case 7: return m_r7;
    case 8: return m_r8;
    case 9: return m_r9;
    case 10: return m_r10;
    case 11: return m_Fp;
    case 12: return m_r12;
    case 13: return m_Sp;
    case 14: return m_Lnk;
    case 15: return m_Pc;
    case 16: return m_Cpsr;
    default: return 0;
  }
}
const char *ARM926EInterruptState::getRegisterName(size_t index) const
{
  return ARM926EInterruptStateRegisterName[index];
}

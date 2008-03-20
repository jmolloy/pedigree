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

const char *MIPS32InterruptStateRegisterName[33] =
{
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
  "gp",
  "sp",
  "fp",
  "ra",
  "SR",
  "Cause",
  "EPC",
  "BadVaddr"
};

size_t MIPS32InterruptState::getRegisterCount() const
{
  return 33;
}
processor_register_t MIPS32InterruptState::getRegister(size_t index) const
{
  switch (index)
  {
    case 0: return m_At;
    case 1: return m_V0;
    case 2: return m_V1;
    case 3: return m_A0;
    case 4: return m_A1;
    case 5: return m_A2;
    case 6: return m_A3;
    case 7: return m_T0;
    case 8: return m_T1;
    case 9: return m_T2;
    case 10: return m_T3;
    case 11: return m_T4;
    case 12: return m_T5;
    case 13: return m_T6;
    case 14: return m_T7;
    case 15: return m_S0;
    case 16: return m_S1;
    case 17: return m_S2;
    case 18: return m_S3;
    case 19: return m_S4;
    case 20: return m_S5;
    case 21: return m_S6;
    case 22: return m_S7;
    case 23: return m_T8;
    case 24: return m_T9;
    case 25: return m_Gp;
    case 26: return m_Sp;
    case 27: return m_Fp;
    case 28: return m_Ra;
    case 29: return m_Sr;
    case 30: return m_Cause;
    case 31: return m_Epc;
    case 32: return m_BadVAddr;
    default: return 0;
  }
}
const char *MIPS32InterruptState::getRegisterName(size_t index) const
{
  return MIPS32InterruptStateRegisterName[index];
}

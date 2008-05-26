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

const char *PPC32InterruptStateRegisterName[39] =
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
  "r11",
  "r12",
  "r13",
  "r14",
  "r15",
  "r16",
  "r17",
  "r18",
  "r19",
  "r20",
  "r21",
  "r22",
  "r23",
  "r24",
  "r25",
  "r26",
  "r27",
  "r28",
  "r29",
  "r30",
  "r31",
  "lr",
  "ctr",
  "cr",
  "srr0",
  "srr1",
  "dsisr",
  "dar"
};

PPC32InterruptState::PPC32InterruptState() :
  m_IntNumber(0), m_Ctr(0), m_Lr(0), m_Cr(0), m_Srr0(0), m_Srr1(0),
  m_Dsisr(0), m_Dar(0),
  m_R0(0), m_R1(0), m_R2(0), m_R3(0), m_R4(0), m_R5(0),
  m_R6(0), m_R7(0), m_R8(0), m_R9(0), m_R10(0), m_R11(0),
  m_R12(0), m_R13(0), m_R14(0), m_R15(0), m_R16(0), m_R17(0),
  m_R18(0), m_R19(0), m_R20(0), m_R21(0), m_R22(0), m_R23(0),
  m_R24(0), m_R25(0), m_R26(0), m_R27(0), m_R28(0), m_R29(0),
  m_R30(0), m_R31(0)
{
}

PPC32InterruptState::PPC32InterruptState(const PPC32InterruptState &is) :
  m_IntNumber(is.m_IntNumber), m_Ctr(is.m_Ctr), m_Lr(is.m_Lr), m_Cr(is.m_Cr), m_Srr0(is.m_Srr0), m_Srr1(is.m_Srr1),
  m_Dsisr(is.m_Dsisr), m_Dar(is.m_Dar),
  m_R0(is.m_R0), m_R1(is.m_R1), m_R2(is.m_R2), m_R3(is.m_R3), m_R4(is.m_R4), m_R5(is.m_R5),
  m_R6(is.m_R6), m_R7(is.m_R7), m_R8(is.m_R8), m_R9(is.m_R9), m_R10(is.m_R10), m_R11(is.m_R11),
  m_R12(is.m_R12), m_R13(is.m_R13), m_R14(is.m_R14), m_R15(is.m_R15), m_R16(is.m_R16), m_R17(is.m_R17),
  m_R18(is.m_R18), m_R19(is.m_R19), m_R20(is.m_R20), m_R21(is.m_R21), m_R22(is.m_R22), m_R23(is.m_R23),
  m_R24(is.m_R24), m_R25(is.m_R25), m_R26(is.m_R26), m_R27(is.m_R27), m_R28(is.m_R28), m_R29(is.m_R29),
  m_R30(is.m_R30), m_R31(is.m_R31)
{
}

PPC32InterruptState & PPC32InterruptState::operator=(const PPC32InterruptState &is)
{
  m_IntNumber = is.m_IntNumber; m_Ctr = is.m_Ctr; m_Lr = is.m_Lr; m_Cr = is.m_Cr; m_Srr0 = is.m_Srr0; m_Srr1 = is.m_Srr1;
  m_Dsisr = is.m_Dsisr; m_Dar = is.m_Dar;
  m_R0 = is.m_R0; m_R1 = is.m_R1; m_R2 = is.m_R2; m_R3 = is.m_R3; m_R4 = is.m_R4; m_R5 = is.m_R5;
  m_R6 = is.m_R6; m_R7 = is.m_R7; m_R8 = is.m_R8; m_R9 = is.m_R9; m_R10 = is.m_R10; m_R11 = is.m_R11;
  m_R12 = is.m_R12; m_R13 = is.m_R13; m_R14 = is.m_R14; m_R15 = is.m_R15; m_R16 = is.m_R16; m_R17 = is.m_R17;
  m_R18 = is.m_R18; m_R19 = is.m_R19; m_R20 = is.m_R20; m_R21 = is.m_R21; m_R22 = is.m_R22; m_R23 = is.m_R23;
  m_R24 = is.m_R24; m_R25 = is.m_R25; m_R26 = is.m_R26; m_R27 = is.m_R27; m_R28 = is.m_R28; m_R29 = is.m_R29;
  m_R30 = is.m_R30; m_R31 = is.m_R31;
  return *this;
}

size_t PPC32InterruptState::getRegisterCount() const
{
  return 39;
}
processor_register_t PPC32InterruptState::getRegister(size_t index) const
{
  switch (index)
  {
    case 0 : return m_R0;
    case 1 : return m_R1;
    case 2 : return m_R2;
    case 3 : return m_R3;
    case 4 : return m_R4;
    case 5 : return m_R5;
    case 6 : return m_R6;
    case 7 : return m_R7;
    case 8 : return m_R8;
    case 9 : return m_R9;
    case 10: return m_R10;
    case 11: return m_R11;
    case 12: return m_R12;
    case 13: return m_R13;
    case 14: return m_R14;
    case 15: return m_R15;
    case 16: return m_R16;
    case 17: return m_R17;
    case 18: return m_R18;
    case 19: return m_R19;
    case 20: return m_R20;
    case 21: return m_R21;
    case 22: return m_R22;
    case 23: return m_R23;
    case 24: return m_R24;
    case 25: return m_R25;
    case 26: return m_R26;
    case 27: return m_R27;
    case 28: return m_R28;
    case 29: return m_R29;
    case 30: return m_R30;
    case 31: return m_R31;
    case 32: return m_Lr;
    case 33: return m_Ctr;
    case 34: return m_Cr;
    case 35: return m_Srr0;
    case 36: return m_Srr1;
    case 37: return m_Dsisr;
    case 38: return m_Dar;
    default: return 0;
  }
}
const char *PPC32InterruptState::getRegisterName(size_t index) const
{
  return PPC32InterruptStateRegisterName[index];
}

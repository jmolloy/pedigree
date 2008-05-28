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

const char *g_pRegisters[32] = {
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
  "r31"};

typedef void (PPCDisassembler::*PPCDelegate)(PPCDisassembler::Instruction,LargeStaticString&);
PPCDelegate ppcLookupTable[] = {
  &PPCDisassembler::null,            // 0x00
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::twi,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::mulli,
  &PPCDisassembler::subfic,
  &PPCDisassembler::null,
  &PPCDisassembler::cmpli,
  &PPCDisassembler::cmpi,
  &PPCDisassembler::addic,
  &PPCDisassembler::addic_dot,
  &PPCDisassembler::addi,
  &PPCDisassembler::addis,

  &PPCDisassembler::bc,            // 0x10
  &PPCDisassembler::sc,
  &PPCDisassembler::b,
  &PPCDisassembler::op13,
  &PPCDisassembler::rlwimi,
  &PPCDisassembler::null,
  &PPCDisassembler::rlwinm,
  &PPCDisassembler::rlwnm,
  &PPCDisassembler::ori,
  &PPCDisassembler::oris,
  &PPCDisassembler::xori,
  &PPCDisassembler::xoris,
  &PPCDisassembler::andi_dot,
  &PPCDisassembler::andis_dot,
  &PPCDisassembler::op1e,
  &PPCDisassembler::op1f,

  &PPCDisassembler::lwz,            // 0x20
  &PPCDisassembler::lwzu,
  &PPCDisassembler::lbz,
  &PPCDisassembler::lbzu,
  &PPCDisassembler::stw,
  &PPCDisassembler::stwu,
  &PPCDisassembler::stb,
  &PPCDisassembler::stbu,
  &PPCDisassembler::lhz,
  &PPCDisassembler::lhzu,
  &PPCDisassembler::lha,
  &PPCDisassembler::lhau,
  &PPCDisassembler::sth,
  &PPCDisassembler::sthu,
  &PPCDisassembler::lmw,
  &PPCDisassembler::stmw,

  &PPCDisassembler::lfs,            // 0x30
  &PPCDisassembler::lfsu,
  &PPCDisassembler::lfd,
  &PPCDisassembler::lfdu,
  &PPCDisassembler::stfs,
  &PPCDisassembler::stfsu,
  &PPCDisassembler::stfd,
  &PPCDisassembler::stfdu,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::op3a,
  &PPCDisassembler::op3b,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::op3e,
  &PPCDisassembler::op3f,
};

PPCDisassembler::PPCDisassembler()
  : m_nLocation(0)
{
}

PPCDisassembler::~PPCDisassembler()
{
}

void PPCDisassembler::setLocation(uintptr_t nLocation)
{
  m_nLocation = nLocation;
}

uintptr_t PPCDisassembler::getLocation()
{
  return m_nLocation;
}

void PPCDisassembler::setMode(size_t nMode)
{
}

void PPCDisassembler::disassemble(LargeStaticString &text)
{
  uint32_t nInstruction = * reinterpret_cast<uint32_t*> (m_nLocation);
  m_nLocation += 4;

  Instruction insn;
  insn.integer = nInstruction;

  PPCDelegate delegate = ppcLookupTable[insn.i.opcode];
  (this->*delegate)(insn, text);
}

#define SIGNED_D_OP(mnemonic) \
  do { \
  text += mnemonic; \
  text += "\t"; \
  text += g_pRegisters[insn.d.d]; \
  text += ","; \
  text += g_pRegisters[insn.d.a]; \
  text += ","; \
text.append (static_cast<int16_t> (insn.d.imm)); } while(0)

#define UNSIGNED_D_OP(mnemonic) \
  do { \
  text += mnemonic; \
  text += "\t"; \
  text += g_pRegisters[insn.d.d]; \
  text += ","; \
  text += g_pRegisters[insn.d.a]; \
  text += ","; \
text.append (static_cast<uint16_t> (insn.d.imm), 16); } while(0)

void PPCDisassembler::null(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  text += "Unrecognised instruction: ";
  text.append(insn.integer, 16);
}

void PPCDisassembler::twi(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  const char *pTrapMnemonic;
  switch (insn.d.d)
  {
    case 1: pTrapMnemonic = "lgt"; break;
    case 2: pTrapMnemonic = "llt"; break;
    case 3: pTrapMnemonic = "lne"; break;
    case 4: pTrapMnemonic = "eq"; break;
    case 5: pTrapMnemonic = "lge"; break;
    case 6: pTrapMnemonic = "lle"; break;
    case 8: pTrapMnemonic = "gt"; break;
    case 0xc: pTrapMnemonic = "ge"; break;
    case 0x10: pTrapMnemonic = "lt"; break;
    case 0x14: pTrapMnemonic = "le"; break;
    case 0x18: pTrapMnemonic = "ne"; break;
    default:
      pTrapMnemonic = "";
  }

  if (insn.d.d == 0x1F) // Unconditional trap
  {
    text += "trap";
  }
  else
  {
    text += "tw";
    text += pTrapMnemonic;
    text += "i\t";
    text += g_pRegisters[insn.d.a];
    text += ",";
    text.append(static_cast<int16_t> (insn.d.imm));
  }
}

void PPCDisassembler::mulli(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  SIGNED_D_OP("mulli");
}

void PPCDisassembler::subfic(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  SIGNED_D_OP("subfic");
}

void PPCDisassembler::cmpli(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  text += "cmpwli\t";
  switch (insn.d.d&0x7)
  {
    case 0: text += ","; break;
    case 1: text += "cr1,"; break;
    case 2: text += "cr2,"; break;
    case 3: text += "cr3,"; break;
    case 4: text += "cr4,"; break;
    case 5: text += "cr5,"; break;
    case 6: text += "cr6,"; break;
    case 7: text += "cr7,"; break;
  }
  text += g_pRegisters[insn.d.a];
  text += ",";
  text.append(static_cast<int16_t> (insn.d.imm));
}

void PPCDisassembler::cmpi(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  text += "cmpwi\t";
  switch ((insn.d.d>>2)&0x7)
  {
    case 0: text += ","; break;
    case 1: text += "cr1,"; break;
    case 2: text += "cr2,"; break;
    case 3: text += "cr3,"; break;
    case 4: text += "cr4,"; break;
    case 5: text += "cr5,"; break;
    case 6: text += "cr6,"; break;
    case 7: text += "cr7,"; break;
  }
  text += g_pRegisters[insn.d.a];
  text += ",";
  text.append(static_cast<int16_t> (insn.d.imm));
}

void PPCDisassembler::addic(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  SIGNED_D_OP("addic");
}

void PPCDisassembler::addic_dot(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  SIGNED_D_OP("addic.");
}

void PPCDisassembler::addi(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  if (insn.d.a == 0) // GPR0?
  {
    text += "li\t";
    text += g_pRegisters[insn.d.d];
    text += ",";
    text.append(static_cast<int16_t> (insn.d.imm));
  }
  else
  {
    SIGNED_D_OP("addi");
  }
}

void PPCDisassembler::addis(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  if (insn.d.a == 0) // GPR0?
  {
    text += "lis\t";
    text += g_pRegisters[insn.d.d];
    text += ",";
    text.append(static_cast<int16_t> (insn.d.imm));
  }
  else
  {
    SIGNED_D_OP("addis");
  }
}

void PPCDisassembler::bc(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  text += "b";
  const char *pMnemonic;
  const char *pLikely = "";
  bool invertCondition = false;
  bool noCondition = false;

  switch (insn.b.bo)
  {
    case 0x00: pMnemonic = "dnz"; invertCondition = true; pLikely = "-"; break;
    case 0x01: pMnemonic = "dnz";  invertCondition = true; pLikely = "+"; break;
    case 0x02: pMnemonic = "dz";  invertCondition = true; pLikely = "-"; break;
    case 0x03: pMnemonic = "dz";  invertCondition = true; pLikely = "+"; break;
    case 0x04: pMnemonic = ""; invertCondition = true; pLikely = "-"; break;
    case 0x05: pMnemonic = ""; invertCondition = true; pLikely = "+"; break;
    case 0x08: pMnemonic = "dnz"; pLikely = "-"; break;
    case 0x09: pMnemonic = "dnz"; pLikely = "+"; break;
    case 0x0a: pMnemonic = "dz"; pLikely = "-"; break;
    case 0x0b: pMnemonic = "dz"; pLikely = "+"; break;
    case 0x0c: pMnemonic = ""; pLikely = "-"; break;
    case 0x0d: pMnemonic = ""; pLikely = "+"; break;
    case 0x10: pMnemonic = "dnz"; noCondition = true; pLikely = "-"; break;
    case 0x11: pMnemonic = "dnz"; noCondition = true; pLikely = "+"; break;
    case 0x12: pMnemonic = "dz"; noCondition = true; pLikely = "-"; break;
    case 0x13: pMnemonic = "dz"; noCondition = true; pLikely = "+"; break;
    case 0x14: pMnemonic = ""; noCondition = true; break;
  }

  const char *pCondition;
  if (!invertCondition)
  {
    switch (insn.b.bi&0x3)
    {
      case 0: pCondition = "lt"; break;
      case 1: pCondition = "gt"; break;
      case 2: pCondition = "eq"; break;
      case 3: pCondition = "so"; break;
    }
  }
  else
  {
    switch (insn.b.bi&0x3)
    {
      case 0: pCondition = "ge"; break;
      case 1: pCondition = "le"; break;
      case 2: pCondition = "ne"; break;
      case 3: pCondition = "ns"; break;
    }
  }

  text += pMnemonic;
  if (!noCondition)
    text += pCondition;
  text += pLikely;

  if (insn.b.lk)
    text += "l";
  if (insn.b.aa)
    text += "a";

  switch (insn.b.bi>>2)
  {
    case 0: text += "\t"; break;
    case 1: text += "\tcr1,"; break;
    case 2: text += "\tcr2,"; break;
    case 3: text += "\tcr3,"; break;
    case 4: text += "\tcr4,"; break;
    case 5: text += "\tcr5,"; break;
    case 6: text += "\tcr6,"; break;
    case 7: text += "\tcr7,"; break;
  }

  int32_t addr = static_cast<int32_t> (insn.b.bd) << 2;
  if (insn.b.aa)
    text.append(static_cast<uint32_t> (addr), 16); // Sign extend with the int32_t cast, then make unsigned with the uint32_t.
  else
    text.append(static_cast<uint32_t> (addr+m_nLocation-4), 16);
  
}

void PPCDisassembler::sc(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  text += "sc";
}

void PPCDisassembler::b(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  text += "b";
  if (insn.i.lk)
    text += "l";
  if (insn.i.aa)
    text += "a";

  int32_t addr = static_cast<uint32_t> (static_cast<int32_t> (insn.i.li)<<2);
  if (!intn.i.aa)
    addr += m_nLocation-4;
  text += "\t,";
  text.append(addr, 16);
}

void PPCDisassembler::op13(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::rlwimi(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::rlwinm(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::rlwnm(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::ori(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::oris(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::xori(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::xoris(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::andi_dot(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::andis_dot(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::op1e(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::op1f(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::lwz(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::lwzu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::lbz(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::lbzu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::stw(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::stwu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::stb(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::stbu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::lhz(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::lhzu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::lha(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::lhau(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::sth(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::sthu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::lmw(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::stmw(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::lfs(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::lfsu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::lfd(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::lfdu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::stfs(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::stfsu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::stfd(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::stfdu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::op3a(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::op3b(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::op3e(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::op3f(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}


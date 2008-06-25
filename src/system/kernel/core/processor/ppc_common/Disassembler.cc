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
#include <Log.h>

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
  &PPCDisassembler::rlwinm,
  &PPCDisassembler::null,
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
    text += mnemonic;                   \
    text.pad(8); \
    text += g_pRegisters[insn.d.d];             \
    text += ",";                                \
    text += g_pRegisters[insn.d.a];             \
    text += ",";                                                \
    text.append (static_cast<int16_t> (insn.d.imm)); } while(0)

#define UNSIGNED_D_OP(mnemonic) \
  do { \
    text += mnemonic;                   \
    text.pad(8); \
    text += g_pRegisters[insn.d.d];             \
    text += ",";                                \
    text += g_pRegisters[insn.d.a];             \
    text += ",";                                                        \
    text.append (static_cast<uint16_t> (insn.d.imm)); } while(0)

#define UNSIGNED_SWAPPED_D_OP(mnemonic) \
  do { \
    text += mnemonic;                   \
    text.pad(8); \
    text += g_pRegisters[insn.d.a];             \
    text += ",";                                \
    text += g_pRegisters[insn.d.d];             \
    text += ",";                                                        \
    text.append (static_cast<uint16_t> (insn.d.imm)); } while(0)

#define LOAD_STORE_D_OP(mnemonic) \
  do { \
    text += mnemonic;                   \
    text.pad(8); \
    text += g_pRegisters[insn.d.d];             \
    text += ",";                                \
    if (insn.d.a == 0) \
    { \
      text += "0x"; \
      text.append (static_cast<uint16_t> (insn.d.imm), 16); \
    } \
    else \
    { \
      text.append (static_cast<int16_t> (insn.d.imm)); \
      text += "("; \
      text += g_pRegisters[insn.d.a];             \
      text += ")";                                                      \
    } } while(0)

#define CR_XO_OP(mnemonic) \
  do { \
    text += mnemonic;                   \
    text.pad(8); \
    text += g_pRegisters[insn.xo.d];             \
    text += ",";                                \
    text += g_pRegisters[insn.xo.a]; \
    text += ","; \
    text += g_pRegisters[insn.xo.b]; \
    } while(0)

#define ARITH_XO_OP(mnemonic) \
  do { \
    text += mnemonic;                   \
    if (insn.xo.oe) text += "o"; \
    if (insn.xo.rc) text += "."; \
    text.pad(8); \
    text += g_pRegisters[insn.xo.d];             \
    text += ",";                                \
    text += g_pRegisters[insn.xo.a]; \
    text += ","; \
    text += g_pRegisters[insn.xo.b]; \
    } while(0)

#define UNARY_ARITH_XO_OP(mnemonic) \
  do { \
    text += mnemonic;                   \
    if (insn.xo.oe) text += "o"; \
    if (insn.xo.rc) text += "."; \
    text.pad(8); \
    text += g_pRegisters[insn.xo.d];             \
    text += ",";                                \
    text += g_pRegisters[insn.xo.a]; \
    } while(0)

#define LOAD_STORE_XO_OP(mnemonic) \
  do { \
    text += mnemonic; \
    text.pad(8); \
    text += g_pRegisters[insn.xo.d]; \
    text += ","; \
    if (insn.xo.a != 0) \
    { \
      text += g_pRegisters[insn.xo.a]; \
      text += ","; \
    } \
    text += g_pRegisters[insn.xo.b]; \
  } while (0)

#define LOGICAL_XO_OP(mnemonic) \
  do { \
    text += mnemonic; \
    if (insn.xo.rc) text += "."; \
    text.pad(8); \
    text += g_pRegisters[insn.xo.a]; \
    text += ","; \
    if (insn.xo.a != 0) \
    { \
      text += g_pRegisters[insn.xo.d]; \
      text += ","; \
    } \
    text += g_pRegisters[insn.xo.b]; \
  } while (0)

static void conditionalBranch(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  const char *pMnemonic;
  const char *pLikely = "";
  bool invertCondition = false;
  bool noCondition = false;

  // TODO: change to v2.00 likely-ness.
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
}

static const char *sprMnemonic(uint32_t spr)
{
  switch (spr)
  {
    case 1: return "xer";
    case 8: return "lr";
    case 9: return "ctr";
    case 18: return "dsisr";
    case 19: return "dar";
    case 22: return "dec";
    case 25: return "sdr1";
    case 26: return "srr0";
    case 27: return "srr1";
    case 272: return "sprg0";
    case 273: return "sprg1";
    case 274: return "sprg2";
    case 275: return "sprg3";
    case 282: return "ear";
    case 287: return "pvr";
    case 528: return "ibat0u";
    case 529: return "ibat0l";
    case 530: return "ibat1u";
    case 531: return "ibat1l";
    case 532: return "ibat2u";
    case 533: return "ibat2l";
    case 534: return "ibat3u";
    case 535: return "ibat3l";
    case 536: return "dbat0u";
    case 537: return "dbat0l";
    case 538: return "dbat1u";
    case 539: return "dbat1l";
    case 540: return "dbat2u";
    case 541: return "dbat2l";
    case 542: return "dbat3u";
    case 543: return "dbat3l";
    default: return 0;
  }
}

void PPCDisassembler::null(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  text += "Unrecognised instruction: ";
  text.append(insn.i.opcode, 16);
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
    text += "i";
    text.pad(8);

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
  text += "cmpwli  ";
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
  text += "cmpwi   ";
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
    text += "li      ";
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
    text += "lis     ";
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

  conditionalBranch(insn, text);

  if (insn.b.lk)
    text += "l";
  if (insn.b.aa)
    text += "a";

  text.pad(8);

  switch (insn.b.bi>>2)
  {
    case 0: text += ""; break;
    case 1: text += "cr1,"; break;
    case 2: text += "cr2,"; break;
    case 3: text += "cr3,"; break;
    case 4: text += "cr4,"; break;
    case 5: text += "cr5,"; break;
    case 6: text += "cr6,"; break;
    case 7: text += "cr7,"; break;
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

  text.pad(8);

  int32_t addr = static_cast<int32_t> (insn.i.li) << 2;
  // Do a manual sign extend because 26 bits is a weird number and GCC can't handle it.
  if (addr & (1<<25))
  {
    addr |= (0x3F)<<26;
  }

  if (!insn.i.aa)
  {
    addr += m_nLocation-4;
  }
  text.append(static_cast<uint32_t> (addr), 16);
}

void PPCDisassembler::op13(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  switch (insn.xo.xo)
  {
    case 0x00: // mcrf
      text += "mcrf    ";
      text += insn.xl.bo;
      text += ",";
      text += insn.xl.bi;
      break;
    case 0x10:
      text += "blr";
      conditionalBranch(insn, text);
      if (insn.i.lk)
      {
        text += "l";
      }
      text.pad (8);
      break;
    case 0x21:
      CR_XO_OP("crnor");
      break;
    case 0x32: // rfi
      text += "rfi";
      break;
    case 0x81:
      CR_XO_OP("crandc");
      break;
    case 0x96:
      text += "isync";
      break;
    case 0xC1:
      CR_XO_OP("crxor");
      break;
    case 0xE1:
      CR_XO_OP("crnand");
      break;
    case 0x101:
      CR_XO_OP("crand");
      break;
    case 0x121:
      CR_XO_OP("creqv");
      break;
    case 0x1A1:
      CR_XO_OP("crorc");
      break;
    case 0x1C1:
      CR_XO_OP("cror");
      break;
    case 0x210: // bcctrx
      text += "bctr";
      conditionalBranch(insn, text);
      if (insn.i.lk)
      {
        text += "l";
      }
      text.pad (8);
      break;
    default:
      text += "Unrecognised opcode 13 XO: ";
      text += insn.xo.xo;
  }
}

void PPCDisassembler::rlwimi(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  text += "rlwimi";
  if (insn.m.rc) text += ".";
  text.pad (8);

  text += g_pRegisters[insn.m.a];
  text += ",(";
  text += g_pRegisters[insn.m.s];
  text += "<<";
  text += insn.m.sh;
  text += ")";

  uint32_t mask = 0;
  if (insn.m.mb < insn.m.me + 1)
  {
    for (int i = insn.m.mb; (i%32) <= insn.m.me; i++)
      mask |= 1>>(i%32);
    text += "&0x";
    text.append(mask, 16);
  }
  else if (insn.m.mb == insn.m.me + 1)
  {
    text += "&0xFFFFFFFF";
  }
  else
  {
    mask = 0xFFFFFFFF;
    for (int i = insn.m.me+1; (i%32) <= insn.m.mb-1; i++)
      mask &= 1>>(i%32);
    text += "&0x";
    text.append(mask, 16);
  }
}

void PPCDisassembler::rlwinm(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  text += "rlwinm";
  if (insn.m.rc) text += ".";
  text.pad (8);

  text += g_pRegisters[insn.m.a];
  text += ",(";
  text += g_pRegisters[insn.m.s];
  text += "<<";
  text += insn.m.sh;
  text += ")";

  uint32_t mask = 0;
  if (insn.m.mb < insn.m.me + 1)
  {
    for (int i = insn.m.mb; i <= insn.m.me; i++)
      mask |= 1<<i;
    text += "&0x";
    text.append(mask, 16);
  }
  else if (insn.m.mb == insn.m.me + 1)
  {
    text += "&0xFFFFFFFF";
  }
  else
  {
    mask = 0xFFFFFFFF;
    for (int i = insn.m.me+1; i <= insn.m.mb-1; i++)
      mask &= 1>>i;
    text += "&0x";
    text.append(mask, 16);
  }
}

void PPCDisassembler::rlwnm(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  text += "rlwnm";
  if (insn.m.rc) text += ".";
  text.pad (8);

  text += g_pRegisters[insn.m.a];
  text += ",(";
  text += g_pRegisters[insn.m.s];
  text += "<<";
  text += insn.m.sh;
  text += ")";

  uint32_t mask = 0;
  if (insn.m.mb < insn.m.me + 1)
  {
    for (int i = insn.m.mb; (i%32) <= insn.m.me; i++)
      mask |= 1>>(i%32);
    text += "&0x";
    text.append(mask, 16);
  }
  else if (insn.m.mb == insn.m.me + 1)
  {
    text += "&0xFFFFFFFF";
  }
  else
  {
    mask = 0xFFFFFFFF;
    for (int i = insn.m.me+1; (i%32) <= insn.m.mb-1; i++)
      mask &= 1>>(i%32);
    text += "&0x";
    text.append(mask, 16);
  }
}

void PPCDisassembler::ori(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  UNSIGNED_SWAPPED_D_OP("ori");
}

void PPCDisassembler::oris(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  UNSIGNED_SWAPPED_D_OP("oris");
}

void PPCDisassembler::xori(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  UNSIGNED_SWAPPED_D_OP("xori");
}

void PPCDisassembler::xoris(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  UNSIGNED_SWAPPED_D_OP("xoris");
}

void PPCDisassembler::andi_dot(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  UNSIGNED_SWAPPED_D_OP("andi.");
}

void PPCDisassembler::andis_dot(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  UNSIGNED_SWAPPED_D_OP("andis.");
}

void PPCDisassembler::op1e(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  // Rotate stuff. Let's ignore for now.
  text += "Unimplemented instruction: ";
  text.append(insn.integer,16);
}

void PPCDisassembler::op1f(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  // Wow, lots of instructions in here.
  switch (insn.xo.xo)
  {
    case 0x000: // cmp
      text += "cmp     ";
      if (insn.xo.d > 0)
      {
        text += "cr";
        text += insn.xo.d;
        text += ",";
      }
      text += g_pRegisters[insn.xo.a];
      text += ",";
      text += g_pRegisters[insn.xo.b];
      break;
    case 0x004:
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
        text.pad(8);

        text += g_pRegisters[insn.d.a];
        text += ",";
        text.append(static_cast<int16_t> (insn.d.imm));
      }
    }
    case 0x008:
      ARITH_XO_OP("subfc");
      break;
    case 0x009:
      ARITH_XO_OP("mulhdu");
      break;
    case 0x00A:
      ARITH_XO_OP("addc");
      break;
    case 0x00B:
      ARITH_XO_OP("mulhwu");
      break;
    case 0x013: // mfcr
      text += "mfcr    ";
      text += g_pRegisters[insn.xo.d];
      break;
    case 0x014: // lwarx
      LOAD_STORE_XO_OP("lwarx");
      break;
    case 0x017: // lwzx
      LOAD_STORE_XO_OP("lwzx");
      break;
    case 0x018:
      LOGICAL_XO_OP("slw");
      break;
    case 0x01A: // cntlzw
      text += "cntlzw";
      if (insn.xo.rc) text += ".";
      text.pad (8);
      text += g_pRegisters[insn.xo.a];
      text += ",";
      text += g_pRegisters[insn.xo.d];
      break;
    case 0x01C:
      LOGICAL_XO_OP("and"); // It's not a shift, but the syntax is the same.
      break;
    case 0x020: // cmpl
      text += "cmpl    ";
      if (insn.xo.d > 0)
      {
        text += "cr";
        text += insn.xo.d;
        text += ",";
      }
      text += g_pRegisters[insn.xo.a];
      text += ",";
      text += g_pRegisters[insn.xo.b];
      break;
    case 0x028:
      ARITH_XO_OP("subf");
      break;
    case 0x036: // dcbst
      text += "dcbst   ";
      if (insn.xo.a != 0)
      {
        text += g_pRegisters[insn.xo.a];
        text += ",";
      }
      text += g_pRegisters[insn.xo.b];
      break;
    case 0x037:
      LOAD_STORE_XO_OP("lwzux");
      break;
    case 0x3C:
      LOGICAL_XO_OP("andc"); // It's not a shift, but the syntax is the same.
      break;
    case 0x04b:
      ARITH_XO_OP("mulhw");
      break;
    case 0x053: // mfmsr
      text += "mfmsr   ";
      text += g_pRegisters[insn.xo.d];
      break;
    case 0x056:
      text += "dcbf    ";
      if (insn.xo.a != 0)
      {
        text += g_pRegisters[insn.xo.a];
        text += ",";
      }
      text += g_pRegisters[insn.xo.b];
      break;
    case 0x057:
      LOAD_STORE_XO_OP("lbzx");
      break;
    case 0x068:
      text += "neg";
      if (insn.xo.oe) text += "o";
      if (insn.xo.rc) text += ".";
      text.pad(8);
      text += g_pRegisters[insn.xo.d];
      text += ",";
      text += g_pRegisters[insn.xo.a];
      break;
    case 0x077:
      LOAD_STORE_XO_OP("lbzux");
      break;
    case 0x07C:
      LOGICAL_XO_OP("nor");
      break;
    case 0x088:
      ARITH_XO_OP("subfe");
      break;
    case 0x08A:
      ARITH_XO_OP("adde");
      break;
    case 0x090: // mtcrf looks like a PITA.
      text += "TODO:: implement mtcrf";
      break;
    case 0x92:
      text += "mtmsr   ";
      text += g_pRegisters[insn.xo.d];
      break;
    case 0x096:
      LOAD_STORE_XO_OP("stwcx");
      break;
    case 0x097:
      LOAD_STORE_XO_OP("stwx");
      break;
    case 0x0B7:
      LOAD_STORE_XO_OP("stwux");
      break;
    case 0x0C8:
      UNARY_ARITH_XO_OP("subfze");
      break;
    case 0x0CA:
      UNARY_ARITH_XO_OP("addze");
      break;
    case 0x0D7:
      LOAD_STORE_XO_OP("stbx");
      break;
    case 0x0E8:
      UNARY_ARITH_XO_OP("subfme");
      break;
    case 0x0EA:
      UNARY_ARITH_XO_OP("addme");
      break;
    case 0x0EB:
      ARITH_XO_OP("mullw");
      break;
    case 0x0F6: // dcbtst
      text += "dcbtst  ";
      text += g_pRegisters[insn.xo.a];
      text += ",";
      text += g_pRegisters[insn.xo.b];
      break;
    case 0x0F7:
      LOAD_STORE_XO_OP("stbux");
      break;
    case 0x10A:
      ARITH_XO_OP("add");
      break;
    case 0x116:
      text += "dcbt  ";
      text += g_pRegisters[insn.xo.a];
      text += ",";
      text += g_pRegisters[insn.xo.b];
      text += ",0x";
      text.append(insn.xo.d, 16);
      break;
    case 0x117:
      LOAD_STORE_XO_OP("lhzx");
      break;
    case 0x11C:
      LOGICAL_XO_OP("eqv");
      break;
    case 0x137:
      LOAD_STORE_XO_OP("lhzux");
      break;
    case 0x13C:
      LOGICAL_XO_OP("xor");
      break;
    case 0x153:
    {
      text += "mf";
      // Split field. Fun.
      uint32_t spr = (insn.xfx.spr&0x1f)<<5;
      spr |= (insn.xfx.spr>>5)&0x1f;

      if (sprMnemonic(spr))
      text += sprMnemonic(spr);
      else
        text += "spr";
      text.pad (8);
      text += g_pRegisters[insn.xfx.d];
      if (!sprMnemonic(spr))
      {
        text += ",";
        text.append (spr);
      }
      break;
    }
    case 0x155:
      LOAD_STORE_XO_OP("lwax");
      break;
    case 0x157:
      LOAD_STORE_XO_OP("lhax");
      break;
    case 0x177:
      LOAD_STORE_XO_OP("lhaux");
      break;
    case 0x197:
      LOAD_STORE_XO_OP("sthx");
      break;
    case 0x19C:
      LOGICAL_XO_OP("orc");
      break;
    case 0x1BC:
      LOGICAL_XO_OP("or");
      break;
    case 0x1CB:
      ARITH_XO_OP("divwu");
      break;
    case 0x1D3: // mtspr
    {
      text += "mt";
      // Split field. Fun.
      uint32_t spr = (insn.xfx.spr&0x1f)<<5;
      spr |= (insn.xfx.spr>>5)&0x1f;

      if (sprMnemonic(spr))
      text += sprMnemonic(spr);
      else
        text += "spr";
      text.pad (8);
      if (!sprMnemonic(spr))
      {
        text.append (spr);
        text += ",";
      }
      text += g_pRegisters[insn.xfx.d];
      break;
    }
    case 0x1D6:
      text += "dcbi    ";
      text += g_pRegisters[insn.xo.a];
      text += ",";
      text += g_pRegisters[insn.xo.b];
      break;
    case 0x1DC:
      LOGICAL_XO_OP("nand");
      break;
    case 0x1EB:
      ARITH_XO_OP("divw");
      break;
    case 0x200:
      text += "mcrxr   cr";
      text += insn.xfx.d;
      break;
    case 0x215:
      LOAD_STORE_XO_OP("lswx");
      break;
    case 0x216:
      LOAD_STORE_XO_OP("lwbrx");
      break;
    case 0x218:
      LOGICAL_XO_OP("srw");
      break;
    case 0x253:
      text += "mfsr    ";
      text += g_pRegisters[insn.xo.d];
      text += ",";
      text += insn.xo.a;
      break;
    case 0x255:
      LOAD_STORE_XO_OP("lswi");
      break;
    case 0x256:
      text += "sync";
      break;
    case 0x293:
      text += "mfsrin  ";
      text += g_pRegisters[insn.xo.d];
      text += ",";
      text += g_pRegisters[insn.xo.b];
      break;
    case 0x295:
      LOAD_STORE_XO_OP("stswx");
      break;
    case 0x296:
      LOAD_STORE_XO_OP("stwbrx");
      break;
    case 0x2D5:
      LOAD_STORE_XO_OP("stswi");
      break;
    case 0x316:
      LOAD_STORE_XO_OP("lhbrx");
      break;
    case 0x338:
      LOGICAL_XO_OP("srawi");
      break;
    case 0x356:
      text += "eieio";
      break;
    case 0x396:
      LOAD_STORE_XO_OP("sthbrx");
      break;
    case 0x39A:
      text += "extsh";
      if (insn.xo.rc) text += ".";
      text.pad (8);
      text += g_pRegisters[insn.xo.a];
      text += ",";
      text += g_pRegisters[insn.xo.d];
      break;
    case 0x3BA:
      text += "extsb";
      if (insn.xo.rc) text += ".";
      text.pad (8);
      text += g_pRegisters[insn.xo.a];
      text += ",";
      text += g_pRegisters[insn.xo.d];
      break;
    case 0x3D6:
      text += "icbi    ";
      text += g_pRegisters[insn.xo.a];
      text += ",";
      text += g_pRegisters[insn.xo.b];
      break;
    case 0x3F6:
      text += "dcbz    ";
      text += g_pRegisters[insn.xo.a];
      text += ",";
      text += g_pRegisters[insn.xo.b];
      break;
    default:
      text += "Unsupported 0x1F instruction, XO: 0x";
      text.append(insn.xo.xo, 16);
      break;
  }
}

void PPCDisassembler::lwz(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("lwz");
}

void PPCDisassembler::lwzu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("lwzu");
}

void PPCDisassembler::lbz(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("lbz");
}

void PPCDisassembler::lbzu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("lbzu");
}

void PPCDisassembler::stw(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("stw");
}

void PPCDisassembler::stwu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("stwu");
}

void PPCDisassembler::stb(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("stb");
}

void PPCDisassembler::stbu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("stbu");
}

void PPCDisassembler::lhz(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("lhz");
}

void PPCDisassembler::lhzu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("lhzu");
}

void PPCDisassembler::lha(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("lha");
}

void PPCDisassembler::lhau(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("lhau");
}

void PPCDisassembler::sth(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("stb");
}

void PPCDisassembler::sthu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("sthu");
}

void PPCDisassembler::lmw(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("lmw");
}

void PPCDisassembler::stmw(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  LOAD_STORE_D_OP("stmw");
}

void PPCDisassembler::lfs(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  null(insn,text);
}

void PPCDisassembler::lfsu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  null(insn,text);
}

void PPCDisassembler::lfd(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  null(insn,text);
}

void PPCDisassembler::lfdu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  null(insn,text);
}

void PPCDisassembler::stfs(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  null(insn,text);
}

void PPCDisassembler::stfsu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  null(insn,text);
}

void PPCDisassembler::stfd(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  null(insn,text);
}

void PPCDisassembler::stfdu(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
  null(insn,text);
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


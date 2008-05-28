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

#ifndef MIPS_DISASSEMBLER_H
#define MIPS_DISASSEMBLER_H

#include <processor/Disassembler.h>

/**
 * A disassembler for R3000-R6000 MIPS32/64 processors.
 * \note Doesn't have 64bit instructions yet.
 */
class PPCDisassembler : public DisassemblerBase
{
public:
  PPCDisassembler();
  ~PPCDisassembler();

  /**
   * Sets the location of the next instruction to be disassembled.
   */
  void setLocation(uintptr_t nLocation);

  /**
   * Gets the location of the next instruction to be disassembled.
   */
  uintptr_t getLocation();

  /**
   * Sets the mode of disassembly - 16-bit, 32-bit or 64-bit
   * If a disassembler doesn't support a requested mode, it should
   * return without changing anything.
   * \param nMode Mode - 16, 32 or 64.
   */
  void setMode(size_t nMode);
  
  /**
   * Disassembles one instruction and populates the given StaticString
   * with a textual representation.
   */
  void disassemble(LargeStaticString &text);

public:
  /**
   * Current disassembling location in memory.
   */
  uintptr_t m_nLocation;

  /** PPC instruction forms */
  struct InstructionFormI
  {
    uint32_t opcode : 6;
    uint32_t li : 24;
    uint32_t aa : 1;
    uint32_t lk : 1;
  };
  struct InstructionFormB
  {
    uint32_t opcode : 6;
    uint32_t bo : 5;
    uint32_t bi : 5;
    uint32_t bd : 14;
    uint32_t aa : 1;
    uint32_t lk : 1;
  };
  struct InstructionFormSC
  {
    uint32_t opcode : 6;
    uint32_t zero : 22;
    uint32_t one : 1;
    uint32_t zero2 : 1;
  };
  struct InstructionFormD
  {
    uint32_t opcode : 6;
    uint32_t d : 5;
    uint32_t a : 5;
    uint32_t imm : 16;
  };
  struct InstructionFormDS
  {
    uint32_t opcode : 6;
    uint32_t d : 5;
    uint32_t a : 5;
    uint32_t ds : 14;
    uint32_t xo : 2;
  };
  struct InstructionFormX
  {
    uint32_t opcode : 6;
    uint32_t d : 5;
    uint32_t s : 5;
    uint32_t b : 5;
    uint32_t xo : 10;
    uint32_t rc : 1;
  };
  struct InstructionFormXL
  {
    uint32_t opcode : 6;
    uint32_t bo : 5;
    uint32_t bi : 5;
    uint32_t crbB : 5;
    uint32_t xo : 10;
    uint32_t lk : 1;
  };
  struct InstructionFormXFX
  {
    uint32_t opcode : 6;
    uint32_t d : 5;
    uint32_t spr : 10;
    uint32_t xo : 10;
    uint32_t zero : 1;
  };
  struct InstructionFormXO
  {
    uint32_t opcode : 6;
    uint32_t d : 5;
    uint32_t a : 5;
    uint32_t b : 5;
    uint32_t oe : 1;
    uint32_t xo : 9;
    uint32_t rc : 1;
  };

  /** Main instruction union */
  union Instruction
  {
    InstructionFormI i;
    InstructionFormB b;
    InstructionFormSC sc;
    InstructionFormD d;
    InstructionFormDS ds;
    InstructionFormX x;
    InstructionFormXL xl;
    InstructionFormXFX xfx;
    InstructionFormXO xo;
    uint32_t integer;
  };

  /** Worker delegates */
  void twi(Instruction insn, LargeStaticString &text);
  void mulli(Instruction insn, LargeStaticString &text);
  void subfic(Instruction insn, LargeStaticString &text);
  void cmpli(Instruction insn, LargeStaticString &text);
  void cmpi(Instruction insn, LargeStaticString &text);
  void addic(Instruction insn, LargeStaticString &text);
  void addic_dot(Instruction insn, LargeStaticString &text);
  void addi(Instruction insn, LargeStaticString &text);
  void addis(Instruction insn, LargeStaticString &text);
  void bc(Instruction insn, LargeStaticString &text);
  void sc(Instruction insn, LargeStaticString &text);
  void b(Instruction insn, LargeStaticString &text);
  void op13(Instruction insn, LargeStaticString &text);
  void rlwimi(Instruction insn, LargeStaticString &text);
  void rlwinm(Instruction insn, LargeStaticString &text);
  void rlwnm(Instruction insn, LargeStaticString &text);
  void ori(Instruction insn, LargeStaticString &text);
  void oris(Instruction insn, LargeStaticString &text);
  void xori(Instruction insn, LargeStaticString &text);
  void xoris(Instruction insn, LargeStaticString &text);
  void andi_dot(Instruction insn, LargeStaticString &text);
  void andis_dot(Instruction insn, LargeStaticString &text);
  void op1e(Instruction insn, LargeStaticString &text);
  void op1f(Instruction insn, LargeStaticString &text);
  void lwz(Instruction insn, LargeStaticString &text);
  void lwzu(Instruction insn, LargeStaticString &text);
  void lbz(Instruction insn, LargeStaticString &text);
  void lbzu(Instruction insn, LargeStaticString &text);
  void stw(Instruction insn, LargeStaticString &text);
  void stwu(Instruction insn, LargeStaticString &text);
  void stb(Instruction insn, LargeStaticString &text);  
  void stbu(Instruction insn, LargeStaticString &text);
  void lhz(Instruction insn, LargeStaticString &text);
  void lhzu(Instruction insn, LargeStaticString &text);
  void lha(Instruction insn, LargeStaticString &text);
  void lhau(Instruction insn, LargeStaticString &text);
  void sth(Instruction insn, LargeStaticString &text);
  void sthu(Instruction insn, LargeStaticString &text);
  void lmw(Instruction insn, LargeStaticString &text);
  void stmw(Instruction insn, LargeStaticString &text);
  void lfs(Instruction insn, LargeStaticString &text);
  void lfsu(Instruction insn, LargeStaticString &text);
  void lfd(Instruction insn, LargeStaticString &text);
  void lfdu(Instruction insn, LargeStaticString &text);
  void stfs(Instruction insn, LargeStaticString &text);
  void stfsu(Instruction insn, LargeStaticString &text);
  void stfd(Instruction insn, LargeStaticString &text);
  void stfdu(Instruction insn, LargeStaticString &text);
  void op3a(Instruction insn, LargeStaticString &text);
  void op3b(Instruction insn, LargeStaticString &text);
  void op3e(Instruction insn, LargeStaticString &text);
  void op3f(Instruction insn, LargeStaticString &text);

  void null(Instruction insn, LargeStaticString &text);
};

typedef PPCDisassembler Disassembler;

#endif

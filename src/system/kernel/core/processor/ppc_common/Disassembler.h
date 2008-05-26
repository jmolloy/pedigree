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
  void integerArithmetic(Instruction insn, LargeStaticString &text);
  void integerCompare(Instruction insn, LargeStaticString &text);
  void integerLogical(Instruction insn, LargeStaticString &text);
  void integerRotate(Instruction insn, LargeStaticString &text);
  void integerShift(Instruction insn, LargeStaticString &text);
  void integerLoad(Instruction insn, LargeStaticString &text);
  void integerStore(Instruction insn, LargeStaticString &text);
  void integerLoadStoreByteReverse(Instruction insn, LargeStaticString &text);
  void integerLoadStoreMultiple(Instruction insn, LargeStaticString &text);
  void integerLoadStoreString(Instruction insn, LargeStaticString &text);
  void memorySync(Instruction insn, LargeStaticString &text);
  void branch(Instruction insn, LargeStaticString &text);
  void conditionRegisterLogical(Instruction insn, LargeStaticString &text);
  void systemLinkage(Instruction insn, LargeStaticString &text);
  void trap(Instruction insn, LargeStaticString &text);
  void processorControl(Instruction insn, LargeStaticString &text);
  void cacheManagement(Instruction insn, LargeStaticString &text);
  void segmentRegisterManipulation(Instruction insn, LargeStaticString &text);
  
  void null(Instruction insn, LargeStaticString &text);
};

typedef PPCDisassembler Disassembler;

#endif

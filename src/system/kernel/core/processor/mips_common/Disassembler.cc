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
#include "Disassembler.h"

const char *g_pOpcodes[64] = {
  0,             // Special
  0,             // RegImm
  "j",
  "jal",
  "beq",
  "bne",
  "blez",
  "bgtz",
  "addi",        // 8
  "addiu",
  "slti",
  "sltiu",
  "andi",
  "ori",
  "xori",
  "lui",
  "cop0",        // 16
  "cop1",
  "cop2",
  "cop3",
  "beql",
  "bnel",
  "blezl",
  "bgtzl",
  0,             // 24
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  "lb",          // 32
  "lh",
  "lwl",
  "lw",
  "lbu",
  "lhu",
  "lwr",
  0,
  "sb",          // 40
  "sh",
  "swl",
  "sw",
  0,
  0,
  "swr",
  "cache",
  "ll",          // 48
  "lwc1",
  "lwc2",
  "lwc3",
  0,
  "ldc1",
  "ldc2",
  "ldc3",
  "sc",          // 56
  "swc1",
  "swc2",
  "swc3",
  0,
  "sdc1",
  "sdc2",
  "sdc3"
};

const char *g_pSpecial[64] = {
  "sll",
  0,
  "srl",
  "sra",
  "sllv",
  0,
  "srlv",
  "srav",
  "jr",          // 8
  "jalr",
  0,
  0,
  "syscall",
  "break",
  0,
  "sync",
  "mfhi",        // 16
  "mthi",
  "mflo",
  "mtlo",
  0,
  0,
  0,
  0,
  "mult",        // 24
  "multu",
  "div",
  "divu",
  0,
  0,
  0,
  0,
  "add",         // 32
  "addu",
  "sub",
  "subu",
  "and",
  "or",
  "xor",
  "nor",
  0,             // 40
  0,
  "slt",
  "sltu",
  0,
  0,
  0,
  0,
  "tge",          // 48
  "tgeu",
  "tlt",
  "tltu",
  "teq",
  0,
  "tne",
  0,
  0,              // 56
  0,
  0,
  0,
  0,
  0,
  0,
  0
};

const char *g_pRegimm[32] = {
  "bltz",
  "bgez",
  "bltzl",
  "bgezl",
  0,
  0,
  0,
  0,
  "tgei",        // 8
  "tgeiu",
  "tlti",
  "tltiu",
  "teqi",
  0,
  "tnei",
  0,
  "bltzal",      // 16
  "bgezal",
  "bltzall",
  "bgezall",
  0,
  0,
  0,
  0,
  0,             // 24
  0,
  0,
  0,
  0,
  0,
  0,
  0
};

const char *g_pCopzRs[32] = {
  "mf",
  0,
  "cf",
  0,
  "mt",
  0,
  "ct",
  0,
  "bc",          // 8
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  "co",           // 16
  "co",
  "co",
  "co",
  "co",
  "co",
  "co",
  "co",
  "co",           // 24
  "co",
  "co",
  "co",
  "co",
  "co",
  "co",
  "co"
};

const char *g_pCopzRt[32] = {
  "bcf",
  "bct",
  "bcfl",
  "bctl",
  0,
  0,
  0,
  0,
  0,              // 8
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,             // 16
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,             // 24
  0,
  0,
  0,
  0,
  0,
  0,
  0
};

const char *g_pCp0Function[64] = {
  0,
  "tlbr",
  "tlbwi",
  0,
  0,
  0,
  "tlbwr",
  0,
  "tlbp",        // 8
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  "rfe",         // 16
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  "eret",        // 24
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,             // 32
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,             // 40
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,             // 48
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,             // 56
  0,
  0,
  0,
  0,
  0,
  0,
  0
};

const char *g_pRegisters[32] = {
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
  "ra"
};

MipsDisassembler::MipsDisassembler()
  : m_nLocation(0)
{
}

MipsDisassembler::~MipsDisassembler()
{
}

void MipsDisassembler::setLocation(uintptr_t nLocation)
{
  m_nLocation = nLocation;
}

uintptr_t MipsDisassembler::getLocation()
{
  return m_nLocation;
}

void MipsDisassembler::setMode(size_t nMode)
{
}

void MipsDisassembler::disassemble(LargeStaticString &text)
{
  uint32_t nInstruction = * reinterpret_cast<uint32_t*> (m_nLocation);
  m_nLocation += 4;

  // Grab the instruction opcode.
  int nOpcode = (nInstruction >> 26) & 0x3F;

  // Handle special opcodes.
  if (nOpcode == 0)
  {
    disassembleSpecial(nInstruction, text);
  }
  else if (nOpcode == 1)
  {
    disassembleRegImm(nInstruction, text);
  }
  else
  {
    disassembleOpcode(nInstruction, text);
  }
}

void MipsDisassembler::disassembleSpecial(uint32_t nInstruction, LargeStaticString & text)
{
  // Special instructions are R-Types.
  uint32_t nRs = (nInstruction >> 21) & 0x1F;
  uint32_t nRt = (nInstruction >> 16) & 0x1F;
  uint32_t nRd = (nInstruction >> 11) & 0x1F;
  uint32_t nShamt = (nInstruction >> 6) & 0x1F;
  uint32_t nFunct = nInstruction & 0x3F;

  switch (nFunct)
  {

    default:
    {
      text += g_pSpecial[nFunct];
      text += " $";
      text += g_pRegisters[nRt];
      text += ", $";
      text += g_pRegisters[nRs];
      text += ", $";
      text += g_pRegisters[nRd];
    }
  };
}

void MipsDisassembler::disassembleRegImm(uint32_t nInstruction, LargeStaticString & text)
{
}

void MipsDisassembler::disassembleOpcode(uint32_t nInstruction, LargeStaticString & text)
{
}

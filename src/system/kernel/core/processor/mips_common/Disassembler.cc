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
  "j     ",
  "jal   ",
  "beq   ",
  "bne   ",
  "blez  ",
  "bgtz  ",
  "addi  ",        // 8
  "addiu ",
  "slti  ",
  "sltiu ",
  "andi  ",
  "ori   ",
  "xori  ",
  "lui   ",
  "cop0  ",        // 16
  "cop1  ",
  "cop2  ",
  "cop3  ",
  "beql  ",
  "bnel  ",
  "blezl ",
  "bgtzl ",
  0,             // 24
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  "lb    ",          // 32
  "lh    ",
  "lwl   ",
  "lw    ",
  "lbu   ",
  "lhu   ",
  "lwr   ",
  0,
  "sb    ",          // 40
  "sh    ",
  "swl   ",
  "sw    ",
  0,
  0,
  "swr   ",
  "cache ",
  "ll    ",          // 48
  "lwc1  ",
  "lwc2  ",
  "lwc3  ",
  0,
  "ldc1  ",
  "ldc2  ",
  "ldc3  ",
  "sc    ",          // 56
  "swc1  ",
  "swc2  ",
  "swc3  ",
  0,
  "sdc1  ",
  "sdc2  ",
  "sdc3  "
};

const char *g_pSpecial[64] = {
  "sll   ",
  0,
  "srl   ",
  "sra   ",
  "sllv  ",
  0,
  "srlv  ",
  "srav  ",
  "jr    ",          // 8
  "jalr  ",
  0,
  0,
  "syscall",
  "break ",
  0,
  "sync  ",
  "mfhi  ",        // 16
  "mthi  ",
  "mflo  ",
  "mtlo  ",
  0,
  0,
  0,
  0,
  "mult  ",        // 24
  "multu ",
  "div   ",
  "divu  ",
  0,
  0,
  0,
  0,
  "add   ",         // 32
  "addu  ",
  "sub   ",
  "subu  ",
  "and   ",
  "or    ",
  "xor   ",
  "nor   ",
  0,             // 40
  0,
  "slt   ",
  "sltu  ",
  0,
  0,
  0,
  0,
  "tge   ",          // 48
  "tgeu  ",
  "tlt   ",
  "tltu  ",
  "teq   ",
  0,
  "tne   ",
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
  "bltz  ",
  "bgez  ",
  "bltzl ",
  "bgezl ",
  0,
  0,
  0,
  0,
  "tgei  ",        // 8
  "tgeiu ",
  "tlti  ",
  "tltiu ",
  "teqi  ",
  0,
  "tnei  ",
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
  "mfc",
  0,
  "cfc",
  0,
  "mtc",
  0,
  "ctc",
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
  "f",
  "t",
  "fl",
  "tl",
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

  // SLL $zero, $zero, 0 == nop.
  if (nInstruction == 0)
  {
    text += "nop";
    return;
  }
  
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

  if (g_pSpecial[nFunct] == 0)
    return;
  
  switch (nFunct)
  {
    case 00: // SLL
    case 02: // SRL
    case 03: // SRA
      text += g_pSpecial[nFunct];
      text += " ";
      text += g_pRegisters[nRd];
      text += ", ";
      text += g_pRegisters[nRt];
      text += ", ";
      text += nShamt;
      break;
    case 04: // SLLV
    case 06: // SRLV
    case 07: // SRAV
      text += g_pSpecial[nFunct];
      text += " ";
      text += g_pRegisters[nRd];
      text += ", ";
      text += g_pRegisters[nRt];
      text += ", ";
      text += g_pRegisters[nRs];
      break;
    case 010: // JR
      text += "jr     ";
      text += g_pRegisters[nRs];
      break;
    case 011: // JALR
    {
      text += "jalr   ";
      if (nRd != 31)
      {
        text += g_pRegisters[nRd];
        text += ", ";
      }
      text += g_pRegisters[nRs];
      break;
    }
    case 014: // SYSCALL
    case 015: // BREAK
    case 017: // SYNC
      text += g_pSpecial[nFunct];
      break;
    case 020: // MFHI
    case 022: // MFLO
      text += g_pSpecial[nFunct];
      text += " ";
      text += g_pRegisters[nRd];
      break;
    case 021: // MTHI
    case 023: // MTLO
      text += g_pSpecial[nFunct];
      text += " ";
      text += g_pRegisters[nRs];
      break;
    case 030: // MULT
    case 031: // MULTU
    case 032: // DIV
    case 033: // DIVU
    case 060: // TGE
    case 061: // TGEU
    case 062: // TLT
    case 063: // TLTU
    case 064: // TEQ
    case 065: // TNE
      text += g_pSpecial[nFunct];
      text += " ";
      text += g_pRegisters[nRs];
      text += ", ";
      text += g_pRegisters[nRt];
      break;
    case 041: // ADDU
      if (nRt == 0)
      {
        // If this is an add of zero, it is actually a "move".
        text += "move   ";
        text += g_pRegisters[nRd];
        text += ", ";
        text += g_pRegisters[nRs];
        break;
      }
      // Fall through.
    default:
    {
      text += g_pSpecial[nFunct];
      text += " ";
      text += g_pRegisters[nRd];
      text += ", ";
      text += g_pRegisters[nRs];
      text += ", ";
      text += g_pRegisters[nRt];
    }
  };
}

void MipsDisassembler::disassembleRegImm(uint32_t nInstruction, LargeStaticString & text)
{
  uint32_t nRs = (nInstruction >> 21)&0x1F;
  uint32_t nOp = (nInstruction >> 16)&0x1F;
  uint16_t nImmediate = nInstruction & 0xFFFF;
  uint32_t nTarget = (nImmediate << 2) + m_nLocation;
  
  switch (nOp)
  {
    case 010: // TGEI
    case 011: // TGEIU
    case 012: // TLTI
    case 013: // TLTIU
    case 014: // TEQI
    case 016: // TNEI
      text += g_pRegimm[nOp];
      text += " ";
      text += g_pRegisters[nRs];
      text += ", ";
      text.append(static_cast<int16_t>(nImmediate), 10);
      break;
    default:
      text += g_pRegimm[nOp];
      text += " ";
      text += g_pRegisters[nRs];
      text += ", 0x";
      text.append(nTarget, 16);
  };
  
}

void MipsDisassembler::disassembleOpcode(uint32_t nInstruction, LargeStaticString & text)
{
  // Opcode instructions are J-Types, or I-Types.
  // Jump target is the lower 26 bits shifted left 2 bits, OR'd with the high 4 bits of the delay slot.
  uint32_t nTarget = ((nInstruction & 0x03FFFFFF)<<2) | (m_nLocation & 0xF0000000);
  uint16_t nImmediate = nInstruction & 0x0000FFFF;
  uint32_t nRt = (nInstruction >> 16) & 0x1F;
  uint32_t nRs = (nInstruction >> 21) & 0x1F;
  int nOpcode = (nInstruction >> 26) & 0x3F;
  
  switch (nOpcode)
  {
    case 02: // J
    case 03: // JAL
      text += g_pOpcodes[nOpcode];
      text += " 0x";
      text.append(nTarget, 16);
      break;
    case 006: // BLEZ
    case 007: // BGTZ
    case 026: // BLEZL
    case 027: // BGTZL
      text += g_pOpcodes[nOpcode];
      text += " ";
      text += g_pRegisters[nRs];
      text += ", 0x";
      text.append( (static_cast<uint32_t>(nImmediate) << 2)+m_nLocation, 16);
      break;
    case 004: // BEQ
    case 005: // BNE
    case 024: // BEQL
    case 025: // BNEL
      text += g_pOpcodes[nOpcode];
      text += " ";
      text += g_pRegisters[nRs];
      text += ", ";
      text += g_pRegisters[nRt];
      text += ", 0x";
      text.append( (static_cast<uint32_t>(nImmediate) << 2)+m_nLocation, 16);
      break;
    case 010: // ADDI
    case 011: // ADDIU
    case 012: // SLTI
    case 013: // SLTIU
    case 014: // ANDI
    case 015: // ORI
    case 016: // XORI
      text += g_pOpcodes[nOpcode];
      text += " ";
      text += g_pRegisters[nRt];
      text += ", ";
      text += g_pRegisters[nRs];
      text += ", ";
      text.append(static_cast<short>(nImmediate), 10);
      break;
    case 017: // LUI
      text += "lui ";
      text += g_pRegisters[nRt];
      text += ", 0x";
      text.append(nImmediate, 16);
      break;
    case 020: // COP0
    case 021: // COP1
    case 022: // COP2
    case 023: // COP3
    {
      if (nRs == 8) // BC
      {
        text += g_pCopzRs[nRs];
        text.append( static_cast<unsigned char>(nOpcode&0x3));
        text += g_pCopzRt[nRt];
        text += ", 0x";
        text.append( (nImmediate<<2)+m_nLocation, 16);
      }
      else if (nOpcode == 020 /* CP0 */ && nRs >= 16 /* CO */)
      {
        text += g_pCp0Function[nInstruction&0x1F];
      }
      else
      {
        text += g_pCopzRs[nRs];
        text.append( static_cast<unsigned char>(nOpcode&0x3));
        text += " ";
        text += g_pRegisters[nRt];
        text += ", ";
        text.append( ((nInstruction>>11)&0x1F), 10);
      }
      break;
    }
    default:
      text += g_pOpcodes[nOpcode];
      text += " ";
      text += g_pRegisters[nRt];
      text += ", ";
      text.append(static_cast<short>(nImmediate), 10);
      text += "(";
      text += g_pRegisters[nRs];
      text += ")";
      break;
  }
}

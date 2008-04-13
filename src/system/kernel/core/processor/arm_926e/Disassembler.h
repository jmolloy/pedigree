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
#ifndef ARM926E_DISASSEMBLER_H
#define ARM926E_DISASSEMBLER_H

#include <processor/Disassembler.h>

/**
 * A disassembler for ARM926E processors.
 */
class Arm926EDisassembler : public DisassemblerBase
{
public:
  Arm926EDisassembler();
  ~Arm926EDisassembler();

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

private:
  /**
   * Current disassembling location in memory.
   */
  uintptr_t m_nLocation;

  void disassembleSpecial(uint32_t nInstruction, LargeStaticString &text);
  void disassembleRegImm(uint32_t nInstruction, LargeStaticString &text);
  void disassembleOpcode(uint32_t nInstruction, LargeStaticString &text);
};

typedef Arm926EDisassembler Disassembler;

#endif

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
#ifndef PROCESSOR_DISASSEMBLER_H
#define PROCESSOR_DISASSEMBLER_H

#include <processor/types.h>
#include <utilities/StaticString.h>

/**
 * Abstraction of a code disassembler.
 */
class DisassemblerBase
{
public:
  /**
   * Destructor does nothing.
   */
  virtual ~DisassemblerBase() {};
  
  /**
   * Sets the location of the next instruction to be disassembled.
   */
  virtual void setLocation(uintptr_t nLocation) =0;

  /**
   * Gets the location of the next instruction to be disassembled.
   */
  virtual uintptr_t getLocation() =0;

  /**
   * Sets the mode of disassembly - 16-bit, 32-bit or 64-bit
   * If a disassembler doesn't support a requested mode, it should
   * return without changing anything.
   * \param nMode Mode - 16, 32 or 64.
   */
  virtual void setMode(size_t nMode) =0;
  
  /**
   * Disassembles one instruction and populates the given StaticString
   * with a textual representation.
   */
  virtual void disassemble(LargeStaticString &text) =0;
  
protected:
  DisassemblerBase() {};
};

#ifdef X86_COMMON
#include <core/processor/x86_common/Disassembler.h>
#endif
#ifdef MIPS_COMMON
#include <core/processor/mips_common/Disassembler.h>
#endif
#ifdef ARM926E
#include <core/processor/arm_common/Disassembler.h>
#endif

#endif

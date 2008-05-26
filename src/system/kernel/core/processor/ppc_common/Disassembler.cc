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
typedef void (PPCDisassembler::*PPCDelegate)(PPCDisassembler::Instruction,LargeStaticString&);
PPCDelegate ppcLookupTable[] = {
  &PPCDisassembler::null,            // 0x00
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::integerArithmetic,
  &PPCDisassembler::integerArithmetic,
  &PPCDisassembler::integerArithmetic,
  &PPCDisassembler::integerArithmetic,

  &PPCDisassembler::null,            // 0x10
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::integerArithmetic,

  &PPCDisassembler::null,            // 0x20
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,

  &PPCDisassembler::null,            // 0x30
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
  &PPCDisassembler::null,
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

void PPCDisassembler::integerArithmetic(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::integerCompare(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::integerLogical(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::integerRotate(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::integerShift(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::integerLoad(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::integerStore(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::integerLoadStoreByteReverse(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::integerLoadStoreMultiple(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::integerLoadStoreString(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::memorySync(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::branch(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::conditionRegisterLogical(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::systemLinkage(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::trap(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::processorControl(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::cacheManagement(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::segmentRegisterManipulation(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}

void PPCDisassembler::null(PPCDisassembler::Instruction insn, LargeStaticString &text)
{
}


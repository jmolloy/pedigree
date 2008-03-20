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
#include <DwarfUnwinder.h>
#include <DwarfCfiAutomaton.h>
#include <DwarfState.h>
#include <Log.h>

DwarfUnwinder::DwarfUnwinder(uintptr_t nData, size_t nLength) :
  m_nData(nData),
  m_nLength(nLength)
{
}

DwarfUnwinder::~DwarfUnwinder()
{
}

bool DwarfUnwinder::unwind(const ProcessorState &inState, ProcessorState &outState)
{
  // Construct a DwarfState object and populate it.
  DwarfState startState;
  
  // Unfortunately the next few lines are highly architecture dependent.
#ifdef X86
  startState.m_R[DWARF_REG_EAX] = inState.eax;
  startState.m_R[DWARF_REG_EBX] = inState.ebx;
  startState.m_R[DWARF_REG_ECX] = inState.ecx;
  startState.m_R[DWARF_REG_EDX] = inState.edx;
  startState.m_R[DWARF_REG_ESI] = inState.esi;
  startState.m_R[DWARF_REG_EDI] = inState.edi;
  startState.m_R[DWARF_REG_ESP] = inState.esp;
  startState.m_R[DWARF_REG_EBP] = inState.ebp;
#endif
  
  // For each CIE or FDE...
  size_t nIndex = 0;
  while (nIndex < m_nLength)
  {
    // Get the length of this entry.
#ifdef BITS_32
    uint32_t nLength = * reinterpret_cast<uint32_t*> (m_nData+nIndex);
    nIndex += sizeof(uint32_t);
    const uint32_t k_nCieId = 0xFFFFFFFF;
#endif
#ifdef BITS_64
    // Initial length field is 96 bytes long.
#error DWARF64 not supported.
#endif
    
    // Get the type of this entry (or CIE pointer if this is a FDE).
    processor_register_t nCie = * reinterpret_cast<processor_register_t*> (m_nData+nIndex);
    nIndex += sizeof(processor_register_t);
    
    // Is this a CIE?
    if (nCie == k_nCieId)
    {
      // Skip over everything.
      nIndex += nLength - sizeof(processor_register_t);
      continue;
    }
    
    // This is a FDE. Get its initial location.
    uintptr_t nInitialLocation = * reinterpret_cast<uintptr_t*> (m_nData+nIndex);
    nIndex += sizeof(uintptr_t);
    
    // Get its addressing range.
    size_t nAddressRange = * reinterpret_cast<size_t*> (m_nData+nIndex);
    nIndex += sizeof(size_t);
    
    uintptr_t nInstructionStart = static_cast<uintptr_t> (nIndex);
    size_t nInstructionLength = nLength - sizeof(processor_register_t) - sizeof(uintptr_t) -
        sizeof(size_t);
    
    // Are we in this range?
    if ((inState.eip < nInitialLocation) ||
        (inState.eip >= nInitialLocation+nAddressRange))
    {
      nIndex += nInstructionLength;
      continue;
    }
    
    // This is a FDE. Get the CIE it corresponds to.
#ifdef BITS_32
    uint32_t nCieEnd = * reinterpret_cast<uint32_t*> (m_nData+nCie) + nCie;
    nCie += sizeof(uint32_t);
#endif
#ifdef BITS_64
    // Initial length field is 96 bytes long.
#error DWARF64 not supported.
#endif
    
    // Ensure our CIE ID is correct.
    processor_register_t nCieId = * reinterpret_cast<processor_register_t*> (m_nData+nCie);
    if (nCieId != k_nCieId)
    {
      WARNING ("DwarfUnwinder::unwind - CIE ID incorrect!");
      return false;
    }
    nCie += sizeof(processor_register_t);
    
    nCie += 1; // Increment over version byte.
    
    const char *pAugmentationString = reinterpret_cast<const char*> (m_nData+nCie);
    while (*pAugmentationString++) // Pass over the augmentation string, waiting for a NULL char.
      nCie ++;
    
    uint8_t *pData = reinterpret_cast<uint8_t*> (m_nData);
    uint32_t nCodeAlignmentFactor   = decodeUleb128(pData, nCie);
    uint32_t nDataAlignmentFactor   = decodeSleb128(pData, nCie);
    uint32_t nReturnAddressRegister = decodeUleb128(pData, nCie);
    
    DwarfCfiAutomaton automaton;
    automaton.initialise (startState, m_nData+nCie, nCieEnd-nCie);
    DwarfState *endState = automaton.execute (nInstructionStart, nInstructionLength, inState.eip);
    
#ifdef X86
    outState.eax = endState->m_R[DWARF_REG_EAX];
    outState.ebx = endState->m_R[DWARF_REG_EBX];
    outState.ecx = endState->m_R[DWARF_REG_ECX];
    outState.edx = endState->m_R[DWARF_REG_EDX];
    outState.esi = endState->m_R[DWARF_REG_ESI];
    outState.edi = endState->m_R[DWARF_REG_EDI];
    outState.esp = endState->m_R[DWARF_REG_ESP];
    outState.ebp = endState->m_R[DWARF_REG_EBP];
    outState.eip = endState->m_Cfa;
#endif
    return true;
  }
  
  return false;
}

uint32_t DwarfUnwinder::decodeUleb128(uint8_t *pBase, uint32_t &nOffset)
{
  uint32_t result = 0;
  uint32_t shift = 0;
  while (true)
  {
    uint8_t byte = pBase[nOffset++];
    result |= (byte&0x7f) << shift;
    if (byte&0x80 == 0)
      break;
    shift += 7;
  }
  return result;
}

int32_t DwarfUnwinder::decodeSleb128(uint8_t *pBase, uint32_t &nOffset)
{
  int32_t result = 0;
  uint32_t shift = 0;
  uint8_t byte;
  while (true)
  {
    byte = pBase[nOffset++];
    result |= (byte&0x7f) << shift;
    if (byte&0x80 == 0)
      break;
    shift += 7;
  }
  if ( (shift < sizeof(int32_t)*8) &&
       (byte & 0x40)) /* If sign bit of byte is set */
    result |= - (1 << shift); /* sign extend */
  return result;
}

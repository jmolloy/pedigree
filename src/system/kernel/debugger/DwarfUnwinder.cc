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

bool DwarfUnwinder::unwind(const ProcessorState &inState, ProcessorState &outState, uintptr_t &frameBase)
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
#ifdef X64
  startState.m_R[DWARF_REG_RAX] = inState.rax;
  startState.m_R[DWARF_REG_RDX] = inState.rdx;
  startState.m_R[DWARF_REG_RCX] = inState.rcx;
  startState.m_R[DWARF_REG_RBX] = inState.rbx;
  startState.m_R[DWARF_REG_RSI] = inState.rsi;
  startState.m_R[DWARF_REG_RDI] = inState.rdi;
  startState.m_R[DWARF_REG_RBP] = inState.rbp;
  startState.m_R[DWARF_REG_RSP] = inState.rsp;
  startState.m_R[DWARF_REG_R8] = inState.r8;
  startState.m_R[DWARF_REG_R9] = inState.r9;
  startState.m_R[DWARF_REG_R10] = inState.r10;
  startState.m_R[DWARF_REG_R11] = inState.r11;
  startState.m_R[DWARF_REG_R12] = inState.r12;
  startState.m_R[DWARF_REG_R13] = inState.r13;
  startState.m_R[DWARF_REG_R14] = inState.r14;
  startState.m_R[DWARF_REG_R15] = inState.r15;
  startState.m_R[DWARF_REG_RFLAGS] = inState.rflags;
#endif
#ifdef MIPS_COMMON
  startState.m_R[DWARF_REG_AT] = inState.m_At;
  startState.m_R[DWARF_REG_V0] = inState.m_V0;
  startState.m_R[DWARF_REG_V1] = inState.m_V1;
  startState.m_R[DWARF_REG_A0] = inState.m_A0;
  startState.m_R[DWARF_REG_A1] = inState.m_A1;
  startState.m_R[DWARF_REG_A2] = inState.m_A2;
  startState.m_R[DWARF_REG_A3] = inState.m_A3;
  startState.m_R[DWARF_REG_T0] = inState.m_T0;
  startState.m_R[DWARF_REG_T1] = inState.m_T1;
  startState.m_R[DWARF_REG_T2] = inState.m_T2;
  startState.m_R[DWARF_REG_T3] = inState.m_T3;
  startState.m_R[DWARF_REG_T4] = inState.m_T4;
  startState.m_R[DWARF_REG_T5] = inState.m_T5;
  startState.m_R[DWARF_REG_T6] = inState.m_T6;
  startState.m_R[DWARF_REG_T7] = inState.m_T7;
  startState.m_R[DWARF_REG_S0] = inState.m_S0;
  startState.m_R[DWARF_REG_S1] = inState.m_S1;
  startState.m_R[DWARF_REG_S2] = inState.m_S2;
  startState.m_R[DWARF_REG_S3] = inState.m_S3;
  startState.m_R[DWARF_REG_S4] = inState.m_S4;
  startState.m_R[DWARF_REG_S5] = inState.m_S5;
  startState.m_R[DWARF_REG_S6] = inState.m_S6;
  startState.m_R[DWARF_REG_S7] = inState.m_S7;
  startState.m_R[DWARF_REG_T8] = inState.m_T8;
  startState.m_R[DWARF_REG_T9] = inState.m_T9;
//   startState.m_R[DWARF_REG_K0] = inState.m_K0;
//   startState.m_R[DWARF_REG_K1] = inState.m_K1;
  startState.m_R[DWARF_REG_GP] = inState.m_Gp;
  startState.m_R[DWARF_REG_SP] = inState.m_Sp;
  startState.m_R[DWARF_REG_FP] = inState.m_Fp;
  startState.m_R[DWARF_REG_RA] = inState.m_Ra;
#endif

  // For each CIE or FDE...
  size_t nIndex = 0;
  while (nIndex < m_nLength)
  {
    // Get the length of this entry.
    uint32_t nLength = * reinterpret_cast<uint32_t*> (m_nData+nIndex);
    nIndex += sizeof(uint32_t);
    const uint32_t k_nCieId = 0xFFFFFFFF;
    
    if (nLength == 0xFFFFFFFF)
    {
      ERROR("64-bit DWARF file detected, but not supported!");
      return false;
    }

    // Get the type of this entry (or CIE pointer if this is a FDE).
    uint32_t nCie = * reinterpret_cast<uint32_t*> (m_nData+nIndex);
    nIndex += sizeof(uint32_t);
    
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
    size_t nInstructionLength = nLength - sizeof(uint32_t) - sizeof(uintptr_t) -
        sizeof(size_t);

    // Are we in this range?
    if ((inState.getInstructionPointer() < nInitialLocation) ||
        (inState.getInstructionPointer() >= nInitialLocation+nAddressRange))
    {
      nIndex += nInstructionLength;
      continue;
    }

    // This is a FDE. Get the CIE it corresponds to.
    uint32_t nCieEnd = * reinterpret_cast<uint32_t*> (m_nData+nCie) + nCie;
    nCie += sizeof(uint32_t);
    nCieEnd += sizeof(uint32_t);
    
    // Ensure our CIE ID is correct.
    uint32_t nCieId = * reinterpret_cast<uint32_t*> (m_nData+nCie);
    if (nCieId != k_nCieId)
    {
      WARNING ("DwarfUnwinder::unwind - CIE ID incorrect!");
      return false;
    }
    nCie += sizeof(uint32_t);
    nCie += 1; // Increment over version byte.

    const char *pAugmentationString = reinterpret_cast<const char*> (m_nData+nCie);
    while (*pAugmentationString++) // Pass over the augmentation string, waiting for a NULL char.
      nCie ++;
    nCie++; // Step over null byte.

    uint8_t *pData = reinterpret_cast<uint8_t*> (m_nData);
    uint32_t nCodeAlignmentFactor   = decodeUleb128(pData, nCie);
    uint32_t nDataAlignmentFactor   = decodeSleb128(pData, nCie);
    uint32_t nReturnAddressRegister = decodeUleb128(pData, nCie);
    
    DwarfCfiAutomaton automaton;
    automaton.initialise (startState, m_nData+nCie, nCieEnd-nCie, nCodeAlignmentFactor,
                          nDataAlignmentFactor, nInitialLocation);
    DwarfState *endState = automaton.execute (m_nData+nInstructionStart, nInstructionLength, inState.getInstructionPointer());
    frameBase = endState->getCfa(startState);
    
#ifdef X86
    outState.eax = endState->getRegister(DWARF_REG_EAX, startState);
    outState.ebx = endState->getRegister(DWARF_REG_EBX, startState);
    outState.ecx = endState->getRegister(DWARF_REG_ECX, startState);
    outState.edx = endState->getRegister(DWARF_REG_EDX, startState);
    outState.esi = endState->getRegister(DWARF_REG_ESI, startState);
    outState.edi = endState->getRegister(DWARF_REG_EDI, startState);
    outState.esp = endState->getCfa(startState); // Architectural rule.
    outState.ebp = endState->getRegister(DWARF_REG_EBP, startState);
    outState.eip = endState->getRegister(nReturnAddressRegister, startState);
#endif
#ifdef X64
    outState.rax = endState->getRegister(DWARF_REG_RAX, startState);
    outState.rdx = endState->getRegister(DWARF_REG_RDX, startState);
    outState.rcx = endState->getRegister(DWARF_REG_RCX, startState);
    outState.rbx = endState->getRegister(DWARF_REG_RBX, startState);
    outState.rsi = endState->getRegister(DWARF_REG_RSI, startState);
    outState.rdi = endState->getRegister(DWARF_REG_RDI, startState);
    outState.rbp = endState->getRegister(DWARF_REG_RBP, startState);
    outState.rsp = endState->getCfa(startState); // Architectural rule.
    outState.r8 = endState->getRegister(DWARF_REG_R8, startState);
    outState.r9 = endState->getRegister(DWARF_REG_R9, startState);
    outState.r10 = endState->getRegister(DWARF_REG_R10, startState);
    outState.r11 = endState->getRegister(DWARF_REG_R11, startState);
    outState.r12 = endState->getRegister(DWARF_REG_R12, startState);
    outState.r13 = endState->getRegister(DWARF_REG_R13, startState);
    outState.r14 = endState->getRegister(DWARF_REG_R14, startState);
    outState.r15 = endState->getRegister(DWARF_REG_R15, startState);
    outState.rflags = endState->getRegister(DWARF_REG_RFLAGS, startState);
    outState.rip = endState->getRegister(nReturnAddressRegister, startState);
#endif
#ifdef MIPS_COMMON
    outState.m_At = endState->getRegister(DWARF_REG_AT, startState);
    outState.m_V0 = endState->getRegister(DWARF_REG_V0, startState);
    outState.m_V1 = endState->getRegister(DWARF_REG_V1, startState);
    outState.m_A0 = endState->getRegister(DWARF_REG_A0, startState);
    outState.m_A1 = endState->getRegister(DWARF_REG_A1, startState);
    outState.m_A2 = endState->getRegister(DWARF_REG_A2, startState);
    outState.m_A3 = endState->getRegister(DWARF_REG_A3, startState);
    outState.m_T0 = endState->getRegister(DWARF_REG_T0, startState);
    outState.m_T1 = endState->getRegister(DWARF_REG_T1, startState);
    outState.m_T2 = endState->getRegister(DWARF_REG_T2, startState);
    outState.m_T3 = endState->getRegister(DWARF_REG_T3, startState);
    outState.m_T4 = endState->getRegister(DWARF_REG_T4, startState);
    outState.m_T5 = endState->getRegister(DWARF_REG_T5, startState);
    outState.m_T6 = endState->getRegister(DWARF_REG_T6, startState);
    outState.m_T7 = endState->getRegister(DWARF_REG_T7, startState);
    outState.m_S0 = endState->getRegister(DWARF_REG_S0, startState);
    outState.m_S1 = endState->getRegister(DWARF_REG_S1, startState);
    outState.m_S2 = endState->getRegister(DWARF_REG_S2, startState);
    outState.m_S3 = endState->getRegister(DWARF_REG_S3, startState);
    outState.m_S4 = endState->getRegister(DWARF_REG_S4, startState);
    outState.m_S5 = endState->getRegister(DWARF_REG_S5, startState);
    outState.m_S6 = endState->getRegister(DWARF_REG_S6, startState);
    outState.m_S7 = endState->getRegister(DWARF_REG_S7, startState);
    outState.m_T8 = endState->getRegister(DWARF_REG_T8, startState);
    outState.m_T9 = endState->getRegister(DWARF_REG_T9, startState);
//     outState.m_K0 = endState->getRegister(DWARF_REG_K0, startState);
//     outState.m_K1 = endState->getRegister(DWARF_REG_K1, startState);
    outState.m_Gp = endState->getRegister(DWARF_REG_GP, startState);
    outState.m_Sp = endState->getCfa(startState); // Architectural rule.
    outState.m_Fp = endState->getRegister(DWARF_REG_FP, startState);
    outState.m_Ra = endState->getRegister(DWARF_REG_RA, startState);
    outState.m_Epc = endState->getRegister(nReturnAddressRegister, startState);
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
    if ((byte&0x80) == 0)
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
    shift += 7;
    if ((byte&0x80) == 0)
      break;
  }
  if ( (shift < sizeof(int32_t)*8) &&
       (byte & 0x40)) /* If sign bit of byte is set */
    result |= - (1 << shift); /* sign extend */
  return result;
}

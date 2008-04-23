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

#include <DwarfCfiAutomaton.h>
#include <DwarfUnwinder.h>
#include <Log.h>

DwarfCfiAutomaton::DwarfCfiAutomaton() :
  m_InitialState(),
  m_CurrentState(),
  m_nCodeAlignmentFactor(),
  m_nDataAlignmentFactor(),
  m_nStartingPc()
{
}

DwarfCfiAutomaton::~DwarfCfiAutomaton()
{
}

void DwarfCfiAutomaton::initialise (DwarfState startingState, uintptr_t nCodeLocation,
                                    size_t nCodeLen, int32_t nCodeAlignmentFactor,
                                    int32_t nDataAlignmentFactor, uintptr_t nStartingPc)
{
  NOTICE("CfiAutomaton starting up at PC=" << Hex << nStartingPc);
  m_nCodeAlignmentFactor = nCodeAlignmentFactor;
  m_nDataAlignmentFactor = nDataAlignmentFactor;
  m_nStartingPc = nStartingPc;

  // Set up our states, initially.
  m_InitialState = startingState;
  m_CurrentState = startingState;
  
  // Execute the preamble instructions.
  execute (nCodeLocation, nCodeLen, static_cast<uintptr_t> (~0));
  
  // Save the state.
  m_InitialState = m_CurrentState;
}

DwarfState *DwarfCfiAutomaton::execute (uintptr_t nCodeLocation, size_t nCodeLen, uintptr_t nBreakAt)
{
  uintptr_t nCurrentCodeLocation = nCodeLocation;
  uintptr_t nProgramCounter = m_nStartingPc;
  while ( (nCurrentCodeLocation < nCodeLocation+nCodeLen) &&
          (nProgramCounter <= nBreakAt) )
  {
    executeInstruction (nCurrentCodeLocation, nProgramCounter);
  }
  // If we reached the end of the instruction stream, this state just continues.
  return &m_CurrentState;
}

void DwarfCfiAutomaton::executeInstruction (uintptr_t &nLocation, uintptr_t &nPc)
{
  uint8_t *pLocation = reinterpret_cast<uint8_t*> (nLocation);
  nLocation++; // nLocation will always increase by at least one.

  if ((pLocation[0] & 0xc0) == DW_CFA_advance_loc)
  {
    uint8_t nDelta = pLocation[0] & 0x3f;
    nPc += nDelta * m_nCodeAlignmentFactor;
    NOTICE("DW_CFA_advance_loc (" << Hex << nDelta << ")");
  }
  else if ((pLocation[0] & 0xc0) == DW_CFA_offset)
  {
    uint8_t nRegister = pLocation[0] & 0x3f;
    uint32_t nLocationOffset = 0;
    pLocation = reinterpret_cast<uint8_t*> (nLocation);
    int32_t nOffset = static_cast<int32_t>(DwarfUnwinder::decodeUleb128(pLocation, nLocationOffset));
    nLocation += nLocationOffset;
    m_CurrentState.m_RegisterStates[nRegister] = DwarfState::Offset;
    m_CurrentState.m_R[nRegister] = static_cast<ssize_t>(nOffset * m_nDataAlignmentFactor);
    NOTICE("DW_CFA_offset (r" << Dec << nRegister << ", " << Hex << nOffset * m_nDataAlignmentFactor << ")");
  }
  else if ((pLocation[0] & 0xc0) == DW_CFA_restore)
  {
    WARNING("DW_CFA_restore not implemented!");
  }
  else switch (pLocation[0])
  {
    case DW_CFA_nop:
    {
//       NOTICE("DW_CFA_nop");
      break;
    }
    case DW_CFA_set_loc:
    {
      processor_register_t *pAddress = reinterpret_cast<processor_register_t*> (nLocation);
      nPc = *pAddress;
      nLocation += sizeof(processor_register_t);
      NOTICE("DW_CFA_set_loc (" << Hex << nPc << ")");
      break;
    }
    case DW_CFA_advance_loc1:
    {
      uint8_t nDelta = * reinterpret_cast<uint8_t*> (nLocation);
      nLocation += 1;
      nPc += nDelta * m_nCodeAlignmentFactor;
      NOTICE("DW_CFA_advance_loc1 (" << Hex << nDelta << ")");
      break;
    }
    case DW_CFA_advance_loc2:
    {
      uint16_t nDelta = * reinterpret_cast<uint16_t*> (nLocation);
      nLocation += 2;
      nPc += nDelta * m_nCodeAlignmentFactor;
      NOTICE("DW_CFA_advance_loc2 (" << Hex << nDelta << ")");
      break;
    }
    case DW_CFA_advance_loc4:
    {
      uint32_t nDelta = * reinterpret_cast<uint32_t*> (nLocation);
      nLocation += 4;
      nPc += nDelta * m_nCodeAlignmentFactor;
      NOTICE("DW_CFA_advance_loc4 (" << Hex << nDelta << ")");
      break;
    }
//     case DW_CFA_offset_extended:
//     {
//     }
//     case DW_CFA_restore_extended:
//     {
//     }
//     case DW_CFA_undefined:
//     {
//     }
//     case DW_CFA_same_value:
//     {
//     }
//     case DW_CFA_register:
//     {
//     }
//     case DW_CFA_remember_state:
//     {
//     }
//     case DW_CFA_restore_state:
//     {
//     }
    case DW_CFA_def_cfa:
    {
      uint32_t nOffset = 0;
      pLocation = reinterpret_cast<uint8_t*> (nLocation);
      m_CurrentState.m_CfaState = DwarfState::ValOffset;
      m_CurrentState.m_CfaRegister = DwarfUnwinder::decodeUleb128(pLocation, nOffset);
      m_CurrentState.m_CfaOffset   = static_cast<ssize_t>(DwarfUnwinder::decodeUleb128(pLocation, nOffset));
      nLocation += nOffset;
      NOTICE("DW_CFA_def_cfa (" << Hex << m_CurrentState.m_CfaRegister << ", " << m_CurrentState.m_CfaOffset << ")");
      break;
    }
    case DW_CFA_def_cfa_register:
    {
      uint32_t nOffset = 0;
      pLocation = reinterpret_cast<uint8_t*> (nLocation);
      m_CurrentState.m_CfaRegister = DwarfUnwinder::decodeUleb128(pLocation, nOffset);
      nLocation += nOffset;
      NOTICE("DW_CFA_def_cfa_reg (" << Hex << m_CurrentState.m_CfaRegister << ")");
      break;
    }
    case DW_CFA_def_cfa_offset:
    {
      uint32_t nOffset = 0;
      pLocation = reinterpret_cast<uint8_t*> (nLocation);
      m_CurrentState.m_CfaOffset = static_cast<ssize_t>(DwarfUnwinder::decodeUleb128(pLocation, nOffset));
      nLocation += nOffset;
      NOTICE("DW_CFA_def_cfa_offset (" << Hex << m_CurrentState.m_CfaOffset << ")");
      break;
    }
//     case DW_CFA_def_cfa_expression:
//     {
//     }
//     case DW_CFA_expression:
//     {
//     }
    case DW_CFA_offset_extended_sf:
    {
      uint32_t nLocationOffset = 0;
      pLocation = reinterpret_cast<uint8_t*> (nLocation);
      uint32_t nRegister = DwarfUnwinder::decodeUleb128(pLocation, nLocationOffset);
      nLocation += nLocationOffset;

      pLocation = reinterpret_cast<uint8_t*> (nLocation);
      int32_t nOffset = DwarfUnwinder::decodeSleb128(pLocation, nLocationOffset);
      nLocation += nLocationOffset;

      m_CurrentState.m_RegisterStates[nRegister] = DwarfState::Offset;
      m_CurrentState.m_R[nRegister] = static_cast<ssize_t>(nOffset * m_nDataAlignmentFactor);
      NOTICE("DW_CFA_offset_extended_sf (r" << Dec << nRegister << ", " << Hex << nOffset * m_nDataAlignmentFactor << ")");
      break;
    }
//     case DW_CFA_def_cfa_sf:
//     {
//     }
//     case DW_CFA_def_cfa_offset_sf:
//     {
//     }
//     case DW_CFA_val_offset:
//     {
//     }
//     case DW_CFA_val_offset_sf:
//     {
//     }
//     case DW_CFA_val_expression:
//     {
//     }
//     case DW_CFA_lo_user:
//     {
//     }
//     case DW_CFA_hi_user:
//     {
//     }
    default:
      ERROR("Unrecognised DWARF CFA instruction: " << Hex << pLocation[0]);
      nPc ++;
  }
}

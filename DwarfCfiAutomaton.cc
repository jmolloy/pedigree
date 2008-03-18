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

DwarfCfiAutomaton::DwarfCfiAutomaton() :
  m_InitialState(),
  m_CurrentState()
{
}

DwarfCfiAutomaton::DwarfCfiAutomaton()
{
}

void DwarfCfiAutomaton::initialise (DwarfState startingState, uintptr_t nCodeLocation, size_t nCodeLen)
{
  // Set up our states, initially.
  m_InitialState = startingState;
  m_CurrentState = startingState;
  
  // Execute the preamble instructions.
  execute (nCodeLocation, nCodeLen, reinterpret_cast<uintptr_t> (~0));
  
  // Save the state.
  m_InitialState = m_CurrentState;
}

DwarfState *DwarfCfiAutomaton::execute (uintptr_t nCodeLocation, size_t nCodeLen, uintptr_t nBreakAt)
{
  uintptr_t nCurrentCodeLocation = nCodeLocation;
  uintptr_t nProgramCounter = 0;
  while ( (nCurrentCodeLocation < nCodeLocation+nCodeLen) &&
          (nProgramCounter != nBreakAt) )
  {
    executeInstruction (nCurrentCodeLocation, nProgramCounter);
  }
}

void DwarfCfiAutomaton::executeInstruction (uintptr_t &nLocation, uintptr_t &nPc)
{
  uint8_t *pLocation = static_cast<uint8_t*> (nLocation);
  
  if (pLocation[0] & 0xc0 == DW_CFA_advance_loc)
  {
  }
  else if (pLocation[0] & 0xc0 == DW_CFA_offset)
  {
  }
  else if (pLocation[0] & 0xc0 == DW_CFA_restore)
  {
  }
  else switch (pLocation[0])
  {
    case DW_CFA_nop:
    {
    }
    case DW_CFA_set_loc:
    {
    }
    case DW_CFA_advance_loc1:
    {
    }
    case DW_CFA_advance_loc2:
    {
    }
    case DW_CFA_advance_loc4:
    {
    }
    case DW_CFA_offset_extended:
    {
    }
    case DW_CFA_restore_extended:
    {
    }
    case DW_CFA_undefined:
    {
    }
    case DW_CFA_same_value:
    {
    }
    case DW_CFA_register:
    {
    }
    case DW_CFA_remember_state:
    {
    }
    case DW_CFA_restore_state:
    {
    }
    case DW_CFA_def_cfa:
    {
    }
    case DW_CFA_def_cfa_register:
    {
    }
    case DW_CFA_def_cfa_offset:
    {
    }
    case DW_CFA_def_cfa_expression:
    {
    }
    case DW_CFA_expression:
    {
    }
    case DW_CFA_offset_extended_sf:
    {
    }
    case DW_CFA_def_cfa_sf:
    {
    }
    case DW_CFA_def_cfa_offset_sf:
    {
    }
    case DW_CFA_val_offset:
    {
    }
    case DW_CFA_val_offset_sf:
    {
    }
    case DW_CFA_val_expression:
    {
    }
    case DW_CFA_lo_user:
    {
    }
    case DW_CFA_hi_user:
    {
    }
    default:
      //ERROR("Unrecognised DWARF CFA instruction: " << Hex << pLocation[0]);
      printf ("Unrecognised DWARF CFA instruction: 0x%x\n", pLocation[0]);
  }
}

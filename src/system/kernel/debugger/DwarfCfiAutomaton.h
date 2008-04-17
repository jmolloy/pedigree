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
#ifndef DWARFCFIAUTOMATON_H
#define DWARFCFIAUTOMATON_H

#include <DwarfState.h>
#include <processor/types.h>

/** @addtogroup kerneldebugger
 * @{ */

// These three are different - they have a delta/register number stored in the lower 6 bits.
#define DW_CFA_advance_loc        0x40
#define DW_CFA_offset             0x80
#define DW_CFA_restore            0xc0

#define DW_CFA_nop                0x00
#define DW_CFA_set_loc            0x01
#define DW_CFA_advance_loc1       0x02
#define DW_CFA_advance_loc2       0x03
#define DW_CFA_advance_loc4       0x04
#define DW_CFA_offset_extended    0x05
#define DW_CFA_restore_extended   0x06
#define DW_CFA_undefined          0x07
#define DW_CFA_same_value         0x08
#define DW_CFA_register           0x09
#define DW_CFA_remember_state     0x0a
#define DW_CFA_restore_state      0x0b
#define DW_CFA_def_cfa            0x0c
#define DW_CFA_def_cfa_register   0x0d
#define DW_CFA_def_cfa_offset     0x0e
#define DW_CFA_def_cfa_expression 0x0f
#define DW_CFA_expression         0x10
#define DW_CFA_offset_extended_sf 0x11
#define DW_CFA_def_cfa_sf         0x12
#define DW_CFA_def_cfa_offset_sf  0x13
#define DW_CFA_val_offset         0x14
#define DW_CFA_val_offset_sf      0x15
#define DW_CFA_val_expression     0x16
#define DW_CFA_lo_user            0x1c
#define DW_CFA_hi_user            0x3f

/**
 * The DWARF debugging standard uses a table for unwinding of the stack.
 * This table has a column for each register and a row for each possible value of the program counter.
 * Obviously, this table would be huge, so instead they encode it using instructions for an imaginary
 * machine.
 * 
 * For each function, a new table is made and initialised. Opcodes cause sequential creation of the table.
 * An extra column is added for the CFA (current frame address - how to find the current frame base) and
 * (possibly) the return address.
 */
class DwarfCfiAutomaton
{
  public:
    /**
     * Constructor - Creates the initial starting state with all registers 'undefined'.
     */
    DwarfCfiAutomaton ();
    /**
     * Destructor - Doesn't do much, as we don't use dynamic memory.
     */
    ~DwarfCfiAutomaton ();
    
    /**
     * Points the automaton to code which it should use to construct the machine starting state.
     * This state is saved for use by a DW_CFA_restore instruction.
     * \param nCodeLocation Location of the CFA instruction stream used to initialise the machine.
     * \param nCodeLen The length (in bytes) of code to execute.
     */
    void initialise (DwarfState startingState, uintptr_t nCodeLocation, size_t nCodeLen,
                     int32_t nCodeAlignmentFactor, int32_t nDataAlignmentFactor,
                     uintptr_t nStartingPc);
    
    /**
     * Executes code at the location given until the instruction pointer passes nCodeLocation+nCodeLen
     * or the instruction pointer equals nBreakAt.
     * \param nCodeLocation Location of the CFA instruction stream to execute.
     * \param nCodeLen Maximum length (in bytes) of code to execute.
     * \param nBreakAt Execution should stop when the table row for this instruction has been constructed.
     * \return The ending state on success, zero on failure.
     */
    DwarfState *execute (uintptr_t nCodeLocation, size_t nCodeLen, uintptr_t nBreakAt);
    
  private:
    /**
     * Execute one instruction from the location given by nLocation, incrementing it to the next.
     */
    void executeInstruction (uintptr_t &nLocation, uintptr_t &nPc);
    
    /**
     * The starting state for this machine.
     */
    DwarfState m_InitialState;
    
    /**
     * The current state of this machine.
     */
    DwarfState m_CurrentState;
    
    /**
     * The code alignment factor.
     */
    int32_t m_nCodeAlignmentFactor;
    
    /**
     * The data alignment factor.
     */
    int32_t m_nDataAlignmentFactor;
    
    /**
     * The initial PC.
     */
    uintptr_t m_nStartingPc;
};

/** @} */

#endif

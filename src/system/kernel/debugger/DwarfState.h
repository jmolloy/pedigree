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

#ifndef DWARFSTATE_H
#define DWARFSTATE_H

#include <processor/types.h>
#include <utilities/utility.h>
#include <Log.h>

/** @addtogroup kerneldebugger
 * @{ */
#ifdef PPC_COMMON
#define DWARF_MAX_REGISTERS 66
#else
#define DWARF_MAX_REGISTERS 50
#endif

#ifdef X86
#define DWARF_REG_EAX 0
#define DWARF_REG_ECX 1
#define DWARF_REG_EDX 2
#define DWARF_REG_EBX 3
#define DWARF_REG_ESP 4
#define DWARF_REG_EBP 5
#define DWARF_REG_ESI 6
#define DWARF_REG_EDI 7
#endif
#ifdef X64
#define DWARF_REG_RAX 0
#define DWARF_REG_RDX 1
#define DWARF_REG_RCX 2
#define DWARF_REG_RBX 3
#define DWARF_REG_RSI 4
#define DWARF_REG_RDI 5
#define DWARF_REG_RBP 6
#define DWARF_REG_RSP 7
#define DWARF_REG_R8 8
#define DWARF_REG_R9 9
#define DWARF_REG_R10 10
#define DWARF_REG_R11 11
#define DWARF_REG_R12 12
#define DWARF_REG_R13 13
#define DWARF_REG_R14 14
#define DWARF_REG_R15 15
#define DWARF_REG_RFLAGS 49
#endif
#ifdef MIPS_COMMON
#define DWARF_REG_ZERO 0
#define DWARF_REG_AT  1
#define DWARF_REG_V0  2
#define DWARF_REG_V1  3
#define DWARF_REG_A0  4
#define DWARF_REG_A1  5
#define DWARF_REG_A2  6
#define DWARF_REG_A3  7
#define DWARF_REG_T0  8
#define DWARF_REG_T1  9
#define DWARF_REG_T2  10
#define DWARF_REG_T3  11
#define DWARF_REG_T4  12
#define DWARF_REG_T5  13
#define DWARF_REG_T6  14
#define DWARF_REG_T7  15
#define DWARF_REG_S0  16
#define DWARF_REG_S1  17
#define DWARF_REG_S2  18
#define DWARF_REG_S3  19
#define DWARF_REG_S4  20
#define DWARF_REG_S5  21
#define DWARF_REG_S6  22
#define DWARF_REG_S7  23
#define DWARF_REG_T8  24
#define DWARF_REG_T9  25
#define DWARF_REG_K0  26
#define DWARF_REG_K1  27
#define DWARF_REG_GP  28
#define DWARF_REG_SP  29
#define DWARF_REG_FP  30
#define DWARF_REG_RA  31
#endif
#ifdef PPC_COMMON
#define DWARF_REG_R0   0
#define DWARF_REG_R1   1
#define DWARF_REG_R2   2
#define DWARF_REG_R3   3
#define DWARF_REG_R4   4
#define DWARF_REG_R5   5
#define DWARF_REG_R6   6
#define DWARF_REG_R7   7
#define DWARF_REG_R8   8
#define DWARF_REG_R9   9
#define DWARF_REG_R10  10
#define DWARF_REG_R11  11
#define DWARF_REG_R12  12
#define DWARF_REG_R13  13
#define DWARF_REG_R14  14
#define DWARF_REG_R15  15
#define DWARF_REG_R16  16
#define DWARF_REG_R17  17
#define DWARF_REG_R18  18
#define DWARF_REG_R19  19
#define DWARF_REG_R20  20
#define DWARF_REG_R21  21
#define DWARF_REG_R22  22
#define DWARF_REG_R23  23
#define DWARF_REG_R24  24
#define DWARF_REG_R25  25
#define DWARF_REG_R26  26
#define DWARF_REG_R27  27
#define DWARF_REG_R28  28
#define DWARF_REG_R29  29
#define DWARF_REG_R30  30
#define DWARF_REG_R31  31
#define DWARF_REG_CR   64
#define DWARF_REG_LR   65 // DWARF standard says this should be 108. G++ differs.
//define DWARF_REG_CTR 109
#endif

// Watch out! Register numbering is seemingly random - x86 and x86_64 ones are here:
// http://wikis.sun.com/display/SunStudio/Dwarf+Register+Numbering
/**
 * Holds one row of a Dwarf CFI table. We technically generate a table, but we only
 * keep track of the current row.
 */
class DwarfState
{
  public:
    enum RegisterState
    {
      SameValue=0,
      Undefined,
      Offset,
      ValOffset,
      Register,
      Expression,
      ValExpression,
      Architectural
    };
    
    DwarfState() :
      m_CfaState(ValOffset),
      m_CfaRegister(0),
      m_CfaOffset(0),
      m_CfaExpression(0),
      m_ReturnAddress(0)
    {
      memset (static_cast<void *> (m_RegisterStates), 0,
              sizeof(RegisterState) * DWARF_MAX_REGISTERS);
      memset (static_cast<void *> (m_R), 0, sizeof(uintptr_t) * DWARF_MAX_REGISTERS);
    }
    ~DwarfState() {}

    /**
     * Copy constructor.
     */
    DwarfState(const DwarfState &other) :
      m_CfaState(other.m_CfaState),
      m_CfaRegister(other.m_CfaRegister),
      m_CfaOffset(other.m_CfaOffset),
      m_CfaExpression(other.m_CfaExpression),
      m_ReturnAddress(other.m_ReturnAddress)
    {
      memcpy (static_cast<void *> (m_RegisterStates),
              static_cast<const void *> (other.m_RegisterStates),
              sizeof(RegisterState) * DWARF_MAX_REGISTERS);
      memcpy (static_cast<void *> (m_R),
              static_cast<const void *> (other.m_R),
              sizeof(uintptr_t) * DWARF_MAX_REGISTERS);
    }

    DwarfState &operator=(const DwarfState &other)
    {
      m_CfaState = other.m_CfaState;
      m_CfaRegister = other.m_CfaRegister;
      m_CfaOffset = other.m_CfaOffset;
      m_CfaExpression = other.m_CfaExpression;
      m_ReturnAddress = other.m_ReturnAddress;
      memcpy (static_cast<void *> (m_RegisterStates),
              static_cast<const void *> (other.m_RegisterStates),
              sizeof(RegisterState) * DWARF_MAX_REGISTERS);
      memcpy (static_cast<void *> (m_R),
              static_cast<const void *> (other.m_R),
              sizeof(uintptr_t) * DWARF_MAX_REGISTERS);
      return *this;
    }
     
    processor_register_t getCfa(const DwarfState &initialState)
    {
      switch (m_CfaState)
      {
        case ValOffset:
        {
          return initialState.m_R[m_CfaRegister] + static_cast<ssize_t> (m_CfaOffset);
        }
        case ValExpression:
        {
          WARNING ("DwarfState::getCfa: Expression type not implemented.");
        }
        default:
          ERROR ("CfaState invalid!");
          return 0;
      }
    }
    
    processor_register_t getRegister(unsigned int nRegister, const DwarfState &initialState)
    {
//       NOTICE("GetRegister: r" << Dec << nRegister);
      switch (m_RegisterStates[nRegister])
      {
        case Undefined:
//           WARNING ("Request for undefined register: r" << Dec << nRegister);
          break;
        case SameValue:
//           WARNING ("SameValue.");
          return initialState.m_R[nRegister];
        case Offset:
        {
//           NOTICE("Offset: " << Hex << getCfa(initialState) << ", " << m_R[nRegister]);
          /// \todo This needs to be better - we need to check if the CFA is borked so we
          ///       don't try to do a stupid read - This requires VirtualAddressSpace, I think.
          if (getCfa(initialState) < 0x2000)
          {
            WARNING("Malformed CFA!");
            return 0x0;
          }
          // "The previous value of this register is saved at the address CFA+N where CFA is the
          //  current CFA value and N is a signed offset."
          return * reinterpret_cast<processor_register_t*>
                     (getCfa(initialState) + static_cast<ssize_t> (m_R[nRegister]));
        }
        case ValOffset:
        {
//           WARNING ("ValOffset.");
          // "The previous value of this register is the value CFA+N where CFA is the current
          //  CFA value and N is a signed offset."
          return static_cast<processor_register_t>
                   (getCfa(initialState) + static_cast<ssize_t> (m_R[nRegister]));
        }
        case Register:
        {
//           WARNING ("Register.");
          // "The previous value of this register is stored in another register numbered R."
          return initialState.m_R[nRegister];
        }
        case Expression:
        {
//           WARNING ("Expression not implemented, r" << Dec << nRegister);
          return 0;
        }
        case ValExpression:
        {
//           WARNING ("ValExpression not implemented, r" << Dec << nRegister);
          return 0;
        }
        case Architectural:
//           WARNING ("Request for 'architectural' register: r" << Dec << nRegister);
          return 0;
        default:
//           ERROR ("DwarfState::getRegister(" << Dec << nRegister << "): Register state invalid.");
          return 0;
      }
    }
    
    /**
     * Register states - these define how to interpret the m_R members.
     */
    RegisterState m_RegisterStates[DWARF_MAX_REGISTERS];
    
    /**
     * Registers (columns in the table).
     */
    processor_register_t m_R[DWARF_MAX_REGISTERS];
      
    /**
     * Current CFA (current frame address) - first and most important column in the table.
     */
    RegisterState m_CfaState;
    /**
     * Current CFA register, and offset.
     */
    uint32_t m_CfaRegister;
    processor_register_t m_CfaOffset;
    /**
     * Current CFA expression, if applicable.
     */
    uint8_t *m_CfaExpression;
    
    /**
     * The column which contains the function return address.
     */
    uintptr_t m_ReturnAddress;

};

/** @} */

#endif

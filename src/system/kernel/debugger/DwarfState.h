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
#ifndef DWARFSTATE_H
#define DWARFSTATE_H

#include <processor/types.h>
#include <utilities/utility.h>

#define DWARF_MAX_REGISTERS 48

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
// Watch out! Register numbering is seemingly random - x86 and x86_64 ones are here:
// http://wikis.sun.com/display/SunStudio/Dwarf+Register+Numbering
/**
 * Holds one row of a Dwarf CFI table. We technically generate a table, but we only
 * keep track of the current row.
 */
class DwarfState
{
  public:
    DwarfState() :
      m_Cfa(0),
      m_CfaRegister(0),
      m_CfaOffset(0),
      m_ReturnAddress(0)
    {
      memset (static_cast<void *> (m_R), 0, sizeof(uintptr_t) * DWARF_MAX_REGISTERS);
    }
    ~DwarfState() {}
    
    /**
     * Registers (columns in the table).
     */
    processor_register_t m_R[DWARF_MAX_REGISTERS];
      
    /**
     * Current CFA (current frame address) - first and most important column in the table.
     */
    processor_register_t m_Cfa;
    /**
     * Current CFA register, and offset.
     */
    uint32_t m_CfaRegister;
    processor_register_t m_CfaOffset;
    
    /**
     * The column which contains the function return address.
     */
    uintptr_t m_ReturnAddress;
};

#endif

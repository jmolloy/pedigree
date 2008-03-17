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

typedef struct
{
  uint32_t reg;   ///< Register number. -1 = no register, -2 = undefined.
  union
  {
    ssize_t s; ///< Signed offset from register.
    size_t u; ///< Unsigned offset from register.
  } off;
} DwarfCfiExpr_t;

/**
 * Holds one row of a Dwarf CFI table. We technically generate a table, but we only
 * keep track of the current row.
 */
class DwarfState
{
  public:
    DwarfState() :
      m_ReturnAddress(0)
    {}
    ~DwarfState() {}
    
    /**
     * Registers (columns in the table).
     */
    DwarfCfiExpr_t m_R[DWARF_MAX_REGISTERS];
      
    /**
     * Current CFA (current frame address) - first and more important column in the table.
     */
    DwarfCfiExpr_t m_Cfa;
    
    /**
     * The column which contains the function return address.
     */
    uintptr_t m_ReturnAddress;
}

#endif

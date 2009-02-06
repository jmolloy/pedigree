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

#include <Elf32.h>
#include <Log.h>
#include <KernelElf.h>

// http://www.caldera.com/developers/devspecs/abi386-4.pdf

#define R_386_NONE     0
#define R_386_32       1
#define R_386_PC32     2
#define R_386_GOT32    3
#define R_386_PLT32    4
#define R_386_COPY     5
#define R_386_GLOB_DAT 6
#define R_386_JMP_SLOT 7
#define R_386_RELATIVE 8
#define R_386_GOTOFF   9
#define R_386_GOTPC    10

bool Elf32::applyRelocation(Elf32Rel_t rel, Elf32SectionHeader_t *pSh, SymbolLookupFn fn)
{
  // Section not loaded?
  if (pSh && pSh->addr == 0)
    return true; // Not a fatal error.

  // Get the address of the unit to be relocated.
  uint32_t address = ((pSh) ? pSh->addr : m_LoadBase) + rel.offset;

  // Addend is the value currently at the given address.
  uint32_t A = * reinterpret_cast<uint32_t*> (address);

  // 'Place' is the address.
  uint32_t P = address;

  // Symbol location.
  uint32_t S = 0;
  Elf32Symbol_t *pSymbols = 0;
  if (!m_pDynamicSymbolTable)
    pSymbols = reinterpret_cast<Elf32Symbol_t*> (m_pSymbolTable->addr);
  else
    pSymbols = m_pDynamicSymbolTable;

  const char *pStringTable = 0;
  if (!m_pDynamicStringTable)
    pStringTable = reinterpret_cast<const char *> (m_pStringTable->addr);
  else
    pStringTable = m_pDynamicStringTable;

  // If this is a section header, patch straight to it.
  if (pSymbols && ELF32_ST_TYPE(pSymbols[ELF32_R_SYM(rel.info)].info) == 3)
  {
    // Section type - the name will be the name of the section header it refers to.
    int shndx = pSymbols[ELF32_R_SYM(rel.info)].shndx;
    Elf32SectionHeader_t *pSh = &m_pSectionHeaders[shndx];
    S = pSh->addr;
  }
  else if (ELF32_R_TYPE(rel.info) != R_386_RELATIVE) // Relative doesn't need a symbol!
  {
    const char *pStr = pStringTable + pSymbols[ELF32_R_SYM(rel.info)].name;
    if (fn == 0)
    {
      S = lookupSymbol(pStr);
      if (S == 0)
        S = KernelElf::instance().globalLookupSymbol(pStr);

      if (S == 0)
        WARNING("Relocation failed for symbol \"" << pStr << "\"");
    }
    else
    {
      S = fn(pStr);
      if (S == 0)
        WARNING("Relocation failed (2) for symbol \"" << pStr << "\"");
    }
  }

  if (S == 0 && (ELF32_R_TYPE(rel.info) != R_386_RELATIVE))
    return false;

  // Base address - commented out to get rid of "unused" warning.
  uint32_t B = m_LoadBase;

  uint32_t result = A; // Currently the result is the addend.
  switch (ELF32_R_TYPE(rel.info))
  {
    case R_386_NONE:
      break;
    case R_386_32:
      result = S + A;
      break;
    case R_386_PC32:
      result = S + A - P;
      break;
    case R_386_JMP_SLOT:
    case R_386_GLOB_DAT:
      result = S;
      break;
    case R_386_COPY:
      NOTICE("386_COPY: S: " << Hex << (uintptr_t)S << ", address: " << (uintptr_t)address);
      result = * reinterpret_cast<uintptr_t*> (S);
      break;
    case R_386_RELATIVE:
      result = B + A;
      break;
    default:
      ERROR ("Relocation not supported: " << Dec << ELF32_R_TYPE(rel.info));
  }

  // Write back the result.
  uint32_t *pResult = reinterpret_cast<uint32_t*> (address);
  *pResult = result;
  return true;
}

bool Elf32::applyRelocation(Elf32Rela_t rela, Elf32SectionHeader_t *pSh, SymbolLookupFn fn)
{
  ERROR("The X86 architecture does not use RELA entries!");
  return false;
}

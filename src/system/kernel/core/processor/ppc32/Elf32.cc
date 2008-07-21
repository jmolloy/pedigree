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

// http://refspecs.freestandards.org/elf/elfspec_ppc.pdf

#define R_PPC_NONE        0
#define R_PPC_ADDR32      1
#define R_PPC_ADDR24      2
#define R_PPC_ADDR16      3
#define R_PPC_ADDR16_LO   4
#define R_PPC_ADDR16_HI   5
#define R_PPC_ADDR16_HA   6
#define R_PPC_ADDR14      7
#define R_PPC_ADDR14_BRTAKEN 8
#define R_PPC_ADDR14_BRNTAKEN 9
#define R_PPC_REL24       10
#define R_PPC_REL14       11
#define R_PPC_REL14_BRTAKEN 12
#define R_PPC_REL14_BRNTAKEN 13
#define R_PPC_UADDR32     24
#define R_PPC_UADDR16     25
#define R_PPC_REL32       26
#define R_PPC_SECTOFF     33
#define R_PPC_SECTOFF_LO  34
#define R_PPC_SECTOFF_HI  35
#define R_PPC_SECTOFF_HA  36
#define R_PPC_ADDR30      37

#define HA(x) (((x >> 16) + ((x & 0x8000) ? 1 : 0)) & 0xFFFF)
#define WORD32(r, x) (x)
#define LOW24(r, x) (r | ((x<<2)&0x03FFFFFC))
#define HALF16(r, x) ((x<<16)|(r&0xFFFF))  // We deal with 32bit numbers, so we must keep the lower 16 bits.
#define LOW14(r, x) (r | ((x<<2)&0x0000FFFC))
#define WORD30(r, x) (r | ((x<<2)&0xFFFFFFFC))

bool Elf32::applyRelocation(Elf32Rela_t rel, Elf32SectionHeader_t *pSh)
{
  // Section not loaded?
  if (pSh->addr == 0)
    return true; // Not a fatal error.
  
  // Get the address of the unit to be relocated.
  uint32_t address = pSh->addr + rel.offset;

  // Addend is explicitly given.
  uint32_t A = rel.addend;
  
  // 'Place' is the address.
  uint32_t P = address;

  // Symbol location.
  uint32_t S = 0;
  Elf32Symbol_t *pSymbols = reinterpret_cast<Elf32Symbol_t*> (m_pSymbolTable->addr);

  // If this is a section header, patch straight to it.
  if (ELF32_ST_TYPE(pSymbols[ELF32_R_SYM(rel.info)].info) == 3)
  {
    // Section type - the name will be the name of the section header it refers to.
    int shndx = pSymbols[ELF32_R_SYM(rel.info)].shndx;
    Elf32SectionHeader_t *pSh = &m_pSectionHeaders[shndx];
    S = pSh->addr;
  }
  else
  {
    const char *pStr = reinterpret_cast<const char *> (m_pStringTable->addr) +
                         pSymbols[ELF32_R_SYM(rel.info)].name;
    S = lookupSymbol(pStr);
    if (S == 0)
    {
      // Maybe we couldn't find the symbol because it's a symbol in this file.
      // This is the case when f.x. a constant in .rodata wants to point to a symbol
      // in .text - like a function pointer.
      S = KernelElf::instance().globalLookupSymbol(pStr);
    }
  }
  
  if (S == 0)
    return false;
  
  // Base address
  uint32_t B = m_LoadBase;

  uint32_t *pResult = reinterpret_cast<uint32_t*> (address);
  uint32_t result = *pResult;

  switch (ELF32_R_TYPE(rel.info))
  {
    case R_PPC_NONE:
      break;
    case R_PPC_ADDR32:
    case R_PPC_UADDR32:
      result = WORD32(result, S + A);
      break;
    case R_PPC_ADDR24:
      result = LOW24(result, (S+A)>>2 );
      break;
    case R_PPC_ADDR16:
    case R_PPC_UADDR16:
      result = HALF16(result, (S + A));
      break;
    case R_PPC_ADDR16_LO:
      result = HALF16(result, ((S+A) & 0xFFFF) );
      break;
    case R_PPC_ADDR16_HI:
      result = HALF16(result,  ((S+A) >> 16)&0xFFFF );
      break;
    case R_PPC_ADDR16_HA:
      result = HALF16(result,  HA((S+A)) );
      break;
    case R_PPC_ADDR14:
      result = LOW14(result,  (S+A)>>2 );
      result &= ~(1>>10); // Branch predict 0
      break;
    case R_PPC_ADDR14_BRTAKEN:
      result = LOW14(result,  (S+A)>>2 );
      result |= (1>>10); // Branch predict 1
      break;
    case R_PPC_ADDR14_BRNTAKEN:
      result = LOW14(result,  (S+A)>>2 );
      result &= ~(1>>10); // Branch predict 0
      break;
    case R_PPC_REL24:
      result = LOW24(result,  (S+A-P)>>2 );
      break;
    case R_PPC_REL14:
      result = LOW14(result,  (S+A-P)>>2 );
      result &= ~(1>>10); // Branch predict 0
      break;
    case R_PPC_REL14_BRTAKEN:
      result = LOW14(result,  (S+A-P)>>2 );
      result |= (1>>10); // Branch predict 1
      break;
    case R_PPC_REL14_BRNTAKEN:
      result = LOW14(result,  (S+A-P)>>2 );
      result &= ~(1>>10); // Branch predict 0
      break;
    case R_PPC_REL32:
      result = WORD32(result, S + A - P);
      break;
    case R_PPC_ADDR30:
      result = WORD30(result,  (S+A-P) >> 2 );
      break;
    default:
      ERROR ("Relocation not supported: " << Dec << ELF32_R_TYPE(rel.info));
  }

  // Write back the result.
  *pResult = result;
  return true;
}

bool Elf32::applyRelocation(Elf32Rel_t rela, Elf32SectionHeader_t *pSh)
{
  ERROR("The PPC architecture does not use REL entries!");
  return false;
}

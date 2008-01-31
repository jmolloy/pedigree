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

#include <Log.h>
#include <Elf32.h>
#include <utility.h>

Elf32::Elf32(char *name) :
  m_pHeader(0),
  m_pSectionHeaders(0),
  m_pStringTable(0),
  m_pSymbolTable(0),
  m_pBuffer(0)
{
  strncpy(id, name, 127);
}

Elf32::~Elf32()
{
}

bool Elf32::load(uint8_t *pBuffer, char *name)
{
  strncpy(id, name, 127);
  
  // The main header will be at pBuffer[0].
  m_pHeader = (Elf32Header_t *) pBuffer;
  
  // Check the ident.
  if ( (m_pHeader->ident[0] != 'E') ||
       (m_pHeader->ident[1] != 'L') ||
       (m_pHeader->ident[2] != 'F') ||
       (m_pHeader->ident[3] != 127) )
  {
    m_pHeader = 0;
    ERROR("ELF file with id '" << name << "'; ident check failed!");
    return false;
  }
  
  // Load in the section headers.
  m_pSectionHeaders = (Elf32SectionHeader_t *) &pBuffer[m_pHeader->shoff];
  
  // Find the string tab&pBuffer[m_pStringTable->offset];le.
  m_pStringTable = &sectionHeaders[m_pHeader->strndx];
  
  // Temporarily load the string table.
  const char *pStrtab = (const char *) &pBuffer[m_pStringTable->offset];
  
  // Go through each section header, trying to find .symtab.
  for (int i = 0; i < m_pHeader->shnum; i++)
  {
    const char *pStr = pStrtab + m_pSectionHeaders[i].name;
    if (!strcmp(pStr, ".symtab"))
    {
      m_pSymbolTable = &sectionHeaders[i];
      break;
    }
  }
  
  if (m_pSymbolTable == 0)
    WARNING("ELF file with id '" << name << "'; symbol table not found!");
  
  m_pBuffer = pBuffer;
  
  // Success.
  return true;
}

bool loadSymbolAndStringHeaders(uint8_t *pStr, uint8_t *pSym)
{
  m_pStringTable = (Elf32SectionHeader_t *)pStr;
  m_pSymbolTable = (Elf32SectionHeader_t *)pSym;
}

bool writeSections()
{
}

unsigned int getLastAddress()
{
}

char *lookupSymbol(uint32_t addr, uint32_t startAddr)
{
}

uint32_t lookupDynamicSymbolAddress(uint32_t off)
{
}

char *lookupDynamicSymbolName(uint32_t off)
{
}

uint32_t getGlobalOffsetTable()
{
}

uint32_t getEntryPoint()
{
}

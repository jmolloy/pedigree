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

#include <Log.h>
#include <Elf64.h>
#include <utilities/utility.h>
#include <BootstrapInfo.h>

Elf64::Elf64() :
  m_pHeader(0),
  m_pSymbolTable(0),
  m_pStringTable(0),
  m_pShstrtab(0),
  m_pGotTable(0),
  m_pRelTable(0),
  m_pDebugTable(0),
  m_pSectionHeaders(0),
  m_nSectionHeaders(0),
  m_pBuffer(0)
{
}

Elf64::~Elf64()
{
}

bool Elf64::load(uint8_t *pBuffer, unsigned int nBufferLength)
{
  // The main header will be at pBuffer[0].
  m_pHeader = reinterpret_cast<Elf64Header_t *>(pBuffer);
  
  // Check the ident.
  if ( (m_pHeader->ident[0] != 'E') ||
       (m_pHeader->ident[1] != 'L') ||
       (m_pHeader->ident[2] != 'F') ||
       (m_pHeader->ident[3] != 127) )
  {
    m_pHeader = 0;
    ERROR("ELF file: ident check failed!");
    return false;
  }
  
  // Load in the section headers.
  m_pSectionHeaders = reinterpret_cast<Elf64SectionHeader_t *>(&pBuffer[m_pHeader->shoff]);
  
  // Find the string table.
  m_pStringTable = &m_pSectionHeaders[m_pHeader->shstrndx];

  // Temporarily load the string table.
  const char *pStrtab = reinterpret_cast<const char *>(&pBuffer[m_pStringTable->offset]);
  
  // Go through each section header, trying to find .symtab.
  for (int i = 0; i < m_pHeader->shnum; i++)
  {
    const char *pStr = pStrtab + m_pSectionHeaders[i].name;
    if (!strcmp(pStr, ".symtab"))
    {
      m_pSymbolTable = &m_pSectionHeaders[i];
      break;
    }
  }
  
  if (m_pSymbolTable == 0)
    WARNING("ELF file: symbol table not found!");
  
  m_pBuffer = pBuffer;
  
  // Success.
  return true;
}

bool Elf64::writeSections()
{
  // TODO
  return false;
}

unsigned int Elf64::getLastAddress()
{
  // TODO
  return 0;
}

const char *Elf64::lookupSymbol(uintptr_t addr, uintptr_t *startAddr)
{
  if (!m_pSymbolTable || !m_pStringTable)
    return 0; // Just return null if we haven't got a symbol table.

  Elf64Symbol_t *pSymbol = reinterpret_cast<Elf64Symbol_t *>(m_pSymbolTable->addr);
  
  const char *pStrtab = reinterpret_cast<const char *>(&m_pBuffer[m_pStringTable->offset]);
  
  for (size_t i = 0; i < m_pSymbolTable->size / sizeof(Elf64Symbol_t); i++)
  {
    // Make sure we're looking at an object or function.
    if (ELF64_ST_TYPE(pSymbol->info) != 0x2 /* function */ &&
        ELF64_ST_TYPE(pSymbol->info) != 0x0 /* notype (asm functions) */)
    {
      pSymbol++;
      continue;
    }
    
    // If we're checking for a symbol that is apparently zero-sized, add one so we can actually
    // count it!
    uint64_t size = pSymbol->size;
    if (size == 0)
      size = 1;
    if ( (addr >= pSymbol->value) &&
          (addr < (pSymbol->value + size)) )
    {
      const char *pStr = pStrtab + pSymbol->name;
      if (startAddr)
        *startAddr = pSymbol->value;
      return pStr;
    }
    pSymbol ++;
  }
  return 0;
  
}

uint32_t Elf64::lookupDynamicSymbolAddress(uint32_t off)
{
  // TODO
  return 0;
}

char *Elf64::lookupDynamicSymbolName(uint32_t off)
{
  // TODO
  return 0;
}

uint32_t Elf64::getGlobalOffsetTable()
{
  // TODO
  return 0;
}

uint32_t Elf64::getEntryPoint()
{
  // TODO
  return 0;
}

uintptr_t Elf64::debugFrameTable()
{
  return m_pDebugTable->addr;
}

uintptr_t Elf64::debugFrameTableLength()
{
  return m_pDebugTable->size;
}

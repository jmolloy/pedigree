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
#include <BootstrapInfo.h>

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

bool Elf32::load(uint8_t *pBuffer)
{
  // The main header will be at pBuffer[0].
  m_pHeader = (Elf32Header_t *) pBuffer;
  
  // Check the ident.
  if ( (m_pHeader->ident[0] != 'E') ||
       (m_pHeader->ident[1] != 'L') ||
       (m_pHeader->ident[2] != 'F') ||
       (m_pHeader->ident[3] != 127) )
  {
    m_pHeader = 0;
    ERROR("ELF file with id '" << id << "'; ident check failed!");
    return false;
  }
  
  // Load in the section headers.
  m_pSectionHeaders = (Elf32SectionHeader_t *) &pBuffer[m_pHeader->shoff];
  
  // Find the string tab&pBuffer[m_pStringTable->offset];le.
  m_pStringTable = &m_pSectionHeaders[m_pHeader->shstrndx];
  
  // Temporarily load the string table.
  const char *pStrtab = (const char *) &pBuffer[m_pStringTable->offset];
  
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
    WARNING("ELF file with id '" << id << "'; symbol table not found!");
  
  m_pBuffer = pBuffer;
  
  // Success.
  return true;
}

bool Elf32::load(BootstrapInfo *pBootstrap)
{
  // Firstly get the section header string table.
  Elf32SectionHeader_t *shstrtab = (Elf32SectionHeader_t *) pBootstrap->getSectionHeader(
                                                          pBootstrap->getStringTable() );
  
  // Normally we will try to use the sectionHeader->offset member to access data, so an
  // Elf section's data can be accessed without being mapped into virtual memory. However,
  // when GRUB loads us, it doesn't tell us where exactly ->offset is with respect to, so here
  // we fix offset = addr, then we work w.r.t 0x00.
  
  // Temporarily load the string table.
  const char *pStrtab = (const char *) shstrtab->addr;

  // Now search for the symbol table.
  for (int i = 0; i < pBootstrap->getSectionHeaderCount(); i++)
  {
    Elf32SectionHeader_t *pSh = (Elf32SectionHeader_t *) pBootstrap->getSectionHeader(i);
    const char *pStr = pStrtab + pSh->name;

    if (pSh->type == SHT_SYMTAB)
    {
      m_pSymbolTable = pSh;
      m_pSymbolTable->offset = m_pSymbolTable->addr;
    }
    else if (!strcmp(pStr, ".strtab"))
    {
      m_pStringTable = pSh;
      m_pStringTable->offset = m_pStringTable->addr;
    }
  }
}

bool Elf32::writeSections()
{
}

unsigned int Elf32::getLastAddress()
{
}

char *Elf32::lookupSymbol(uint32_t addr, uint32_t *startAddr)
{
  if (!m_pSymbolTable || !m_pStringTable)
    return (char *) 0; // Just return null if we haven't got a symbol table.
  
  Elf32Symbol_t *pSymbol = (Elf32Symbol_t *)m_pSymbolTable->addr;
  
  const char *pStrtab = (const char *) &m_pBuffer[m_pStringTable->offset];
  
  for (int i = 0; i < m_pSymbolTable->size / sizeof(Elf32Symbol_t); i++)
  {
    // If we're checking for a symbol that is apparently zero-sized, add one so we can actually
    // count it!
    uint32_t size = pSymbol->size;
    if (size == 0)
      size = 1;
    if ( (addr >= pSymbol->value) &&
         (addr < (pSymbol->value + size)) )
    {
      const char *pStr = pStrtab + pSymbol->name;
      if (startAddr)
        *startAddr = pSymbol->value;
      return (char *) pStr;
    }
    pSymbol ++;
  }
  return 0;
  
}

uint32_t Elf32::lookupDynamicSymbolAddress(uint32_t off)
{
}

char *Elf32::lookupDynamicSymbolName(uint32_t off)
{
}

uint32_t Elf32::getGlobalOffsetTable()
{
}

uint32_t Elf32::getEntryPoint()
{
}

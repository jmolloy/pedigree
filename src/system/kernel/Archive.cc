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

#include "Archive.h"
#include <processor/PhysicalMemoryManager.h>
#include <utilities/StaticString.h>
#include <panic.h>
#include <Log.h>

Archive::Archive(uint8_t *pPhys) :
  m_pPhysicalAddress(pPhys), m_pVirtualAddress(0), m_Region("Archive")
{
  if (reinterpret_cast<uintptr_t> (m_pPhysicalAddress) & 0x1000)
  {
    panic("Archive: Alignment issues");
  }
  bool b = PhysicalMemoryManager::instance().allocateRegion(m_Region,
                                                   0xA00, // 2.5K pages = 10MB
                                                   PhysicalMemoryManager::continuous,
                                                   0, // Flags
                                                   reinterpret_cast<physical_uintptr_t> (m_pPhysicalAddress));
  m_pVirtualAddress = reinterpret_cast<uint8_t*> (m_Region.virtualAddress());
  if (!b)
    ERROR("Archive: allocateRegion failed.");
}

Archive::~Archive()
{
  // TODO destroy all pages.
}

size_t Archive::getNumFiles()
{
  uintptr_t location = reinterpret_cast<uintptr_t> (m_pVirtualAddress);
  File *pFile = reinterpret_cast<File*> (location);
  NormalStaticString str;
  int i = 0;
  while (pFile->name[0] != '\0')
  {
    str = NormalStaticString(pFile->size);
    size_t size = str.intValue(8); // Octal.
    size_t nBlocks = size / 512;
    if (size % 512) nBlocks++;
    location += 512*(nBlocks+1);
    pFile = reinterpret_cast<File*> (location);
    i++;
  }
  return i;
}

size_t Archive::getFileSize(size_t n)
{
  uintptr_t location = reinterpret_cast<uintptr_t> (m_pVirtualAddress);
  File *pFile = reinterpret_cast<File*> (location);
  NormalStaticString str;
  int i = 0;
  while (pFile->name[0] != '\0')
  {
    str = NormalStaticString(pFile->size);
    size_t size = str.intValue(8); // Octal.

    if (i == n)
    {
      return size;
    }
    
    size_t nBlocks = size / 512;
    if (size % 512) nBlocks++;
    location += 512*(nBlocks+1);
    pFile = reinterpret_cast<File*> (location);
    i++;
  }
  return 0;
}

char *Archive::getFileName(size_t n)
{
  uintptr_t location = reinterpret_cast<uintptr_t> (m_pVirtualAddress);
  File *pFile = reinterpret_cast<File*> (location);
  NormalStaticString str;
  int i = 0;
  while (pFile->name[0] != '\0')
  {
    if (i == n)
    {
      return pFile->name;
    }
    str = NormalStaticString(pFile->size);
    size_t size = str.intValue(8); // Octal.
    size_t nBlocks = size / 512;
    if (size % 512) nBlocks++;
    location += 512*(nBlocks+1);
    pFile = reinterpret_cast<File*> (location);
    i++;
  }
  return 0;
}

uintptr_t *Archive::getFile(size_t n)
{
  uintptr_t location = reinterpret_cast<uintptr_t> (m_pVirtualAddress);
  File *pFile = reinterpret_cast<File*> (location);
  NormalStaticString str;
  int i = 0;
  while (pFile->name[0] != '\0')
  {
    if (i == n)
    {
      return reinterpret_cast<uintptr_t*> (location+512);
    }
    str = NormalStaticString(pFile->size);
    size_t size = str.intValue(8); // Octal.
    size_t nBlocks = size / 512;
    if (size % 512) nBlocks++;
    location += 512*(nBlocks+1);
    pFile = reinterpret_cast<File*> (location);
    i++;
  }
  return 0;
}



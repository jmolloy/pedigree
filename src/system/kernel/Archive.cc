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
#include <processor/VirtualAddressSpace.h>
#include <utilities/StaticString.h>
#include <panic.h>
#include <Log.h>

Archive::Archive(uint8_t *pPhys, size_t sSize) :
  m_Region("Archive")
{

  if ((reinterpret_cast<physical_uintptr_t>(pPhys) & (PhysicalMemoryManager::getPageSize() - 1)) != 0)
    panic("Archive: Alignment issues");

  if (PhysicalMemoryManager::instance().allocateRegion(m_Region,
                                                       (sSize + PhysicalMemoryManager::getPageSize() - 1) / PhysicalMemoryManager::getPageSize(),
                                                       PhysicalMemoryManager::continuous,
                                                       VirtualAddressSpace::KernelMode,
                                                       reinterpret_cast<physical_uintptr_t>(pPhys))
      == false)
  {
    ERROR("Archive: allocateRegion failed.");
  }
}

Archive::~Archive()
{
  // TODO destroy all pages.
}

size_t Archive::getNumFiles()
{
  size_t i = 0;
  File *pFile = getFirst();
  while (pFile != 0)
  {
    i++;
    pFile = getNext(pFile);
  }
  return i;
}

size_t Archive::getFileSize(size_t n)
{
  File *pFile = get(n);
  NormalStaticString str(pFile->size);
  return str.intValue(8); // Octal
}

char *Archive::getFileName(size_t n)
{
  return get(n)->name;
}

uintptr_t *Archive::getFile(size_t n)
{
  return reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(get(n)) + 512);
}

Archive::File *Archive::getFirst()
{
  return reinterpret_cast<File*> (m_Region.virtualAddress());
}

Archive::File *Archive::getNext(File *pFile)
{
  NormalStaticString str(pFile->size);
  size_t size = str.intValue(8); // Octal.
  size_t nBlocks = (size + 511) / 512;
  pFile = adjust_pointer(pFile, 512 * (nBlocks + 1));
  if (pFile->name[0] == '\0')return 0;
  return pFile;
}

Archive::File *Archive::get(size_t n)
{
  File *pFile = getFirst();
  for (size_t i = 0;i < n;i++)
    pFile = getNext(pFile);
  return pFile;
}

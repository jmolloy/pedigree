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
#ifndef MEMORYMAP_H
#define MEMORYMAP_H

class MemoryMap
{
public:
  /**
   * Constructor - doesn't need to do much.
   */
  MemoryMap();
  ~MemoryMap();

  /**
   * Registers an ID with the given description string.
   * \return true on success, false if the ID was taken, or was zero.
   * \note Zero is NOT a valid ID.
   */
  bool registerID(unsigned int nId, const char *pStr, class FileLoader *pSymbolLookup=0);
  
  /**
   * Unregisters an ID with its description string.
   */
  void unregisterID(unsigned int nId);
  
  /**
   * Marks a region of memory as 'mapped'. Takes an associated source ID as
   * given to registerID.
   * \return true on success, false if the region is already mapped.
   */
  bool map(unsigned int nStart, unsigned int nLength, unsigned int nId);
  
  /**
   * Marks a region of memory previously 'mapped' as now 'unmapped'.
   * \return true on success, false if no such region existed.
   */
  bool unmap(unsigned int nStart);
  
  /**
   * Returns the ID of the region the given address is in. Returns zero otherwise.
   */
  unsigned int lookup(unsigned int nAddress);
  
  /**
   * Returns the description string for a given ID.
   */
  const char *getDescription(unsigned int nId);
  
  /**
   * Returns the FileLoader for a given ID.
   */
  class FileLoader *getFileLoader(unsigned int nId);
  
};

#endif

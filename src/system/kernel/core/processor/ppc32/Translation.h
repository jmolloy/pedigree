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
#ifndef TRANSLATION_H
#define TRANSLATION_H

#include <processor/types.h>

#define NUM_TRANSLATIONS 256

/**
 * Representation of a translation entry as given by OpenFirmware.
 */
class Translations
{
public:
  struct Translation
  {
    uint32_t virt;
    uint32_t size;
    uint32_t phys;
    uint32_t mode;
  };

  Translations();
  ~Translations();

  /** Obtains the n'th translation. */
  Translation getTranslation(size_t n);

  /** Returns the number of valid translations. */
  size_t getNumTranslations();

  /** Adds a translation to the end of our array.  */
  void addTranslation(uint32_t virt, uint32_t phys, uint32_t size, uint32_t mode);

  /** Attempts to find a free section of physical memory.
      \param size The size of memory to look for, in bytes.
      \return 0 on failure. */
  uint32_t findFreePhysicalMemory(uint32_t size, uint32_t align=0x100000);

  /** Removes any translations with virtual addresses in the range of start..end. */
  void removeRange(uintptr_t start, uintptr_t end);

private:
  /** The main translations array */
  Translation m_pTranslations[NUM_TRANSLATIONS];
  
  /** The current number of valid translations. */
  size_t m_nTranslations;
};
#endif

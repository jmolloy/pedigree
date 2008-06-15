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
#include "Translation.h"
#include <machine/openfirmware/OpenFirmware.h>
#include <machine/openfirmware/Device.h>
#include <Log.h>
#include <panic.h>

Translations::Translations() :
  m_nTranslations(0)
{
  // Get the current translations list.
  OFDevice chosen (OpenFirmware::instance().findDevice("/chosen"));
  OFDevice mmu (chosen.getProperty("mmu"));

  if (mmu.getProperty("translations", m_pTranslations, sizeof(Translation)*NUM_TRANSLATIONS) == -1)
  {
    // We have to panic, because otherwise Processor would try to change
    // page tables with no translation entries in!
    panic("Translations not detected correctly");
    return;
  }

  m_nTranslations = mmu.getPropertyLength("translations") / sizeof(Translation);
  for (unsigned int i = 0; i < m_nTranslations; i++)
  {
    NOTICE("T: " << Hex << m_pTranslations[i].virt << ", " << m_pTranslations[i].phys << ", " << m_pTranslations[i].size << ", " << m_pTranslations[i].mode);
  }
}

Translations::~Translations()
{
}

Translations::Translation Translations::getTranslation(size_t n)
{
  return m_pTranslations[n];
}

size_t Translations::getNumTranslations()
{
  return m_nTranslations;
}

void Translations::addTranslation(uint32_t virt, uint32_t phys, uint32_t size, uint32_t mode)
{
  m_pTranslations[m_nTranslations].virt = virt;
  m_pTranslations[m_nTranslations].phys = phys;
  m_pTranslations[m_nTranslations].size = size;
  m_pTranslations[m_nTranslations].mode = mode;
  m_nTranslations++;
  if (m_nTranslations >= NUM_TRANSLATIONS)
    panic("Too many translations!");
}

/// \todo Buggy, I think.
uint32_t Translations::findFreePhysicalMemory(uint32_t size, uint32_t align)
{
  // Quickly grab the SDR1 value so that we don't accidentally overwrite it.
  uint32_t sdr1;
  asm volatile("mfsdr1 %0" : "=r" (sdr1));
  sdr1 &= 0xFFFFF000;

  // For every page on an 'align' boundary...
  for (uint32_t i = 0x000000; i < 0xFFFFFFFF-align; i += align)
  {
    uint32_t addr = i;
    // Is this address taken?
    bool taken = false;

    for (unsigned int j = 0; j < m_nTranslations; j++)
    {
      // We're looking for the two regions to be disjoint.
      // Start address in our range?
      if ( (m_pTranslations[j].phys >= addr) &&
           (m_pTranslations[j].phys < addr+size))
      {
        taken = true;
        break;
      }
      // End address in our range?
      uint32_t end = m_pTranslations[j].phys+m_pTranslations[j].size;
      if ( (end >= addr) &&
           (end < addr+size))
      {
        taken = true;
        break;
      }
    }
    // Check SDR1
    if ( (sdr1 >= addr && sdr1 < addr+size) ||
         ((sdr1+0x1000000) >= addr && (sdr1+0x1000000) < (addr+size) ))
      taken = true;
    if (!taken) return addr;
  }
  return 0;
}

void Translations::removeRange(uintptr_t start, uintptr_t end)
{
  bool removed = true;
  while (removed)
  {
    removed = false;
    for (unsigned int i = 0; i < m_nTranslations; i++)
    {
      if (m_pTranslations[i].virt >= start && m_pTranslations[i].virt < end)
      {
        // Remove this translation.
        removed = true;
        for (; i < m_nTranslations; i++)
          m_pTranslations[i] = m_pTranslations[i+1];
        m_nTranslations--;
        // Start again.
        break;
      }
    }
  }
}

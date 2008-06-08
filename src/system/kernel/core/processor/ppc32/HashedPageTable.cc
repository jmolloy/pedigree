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

#include "HashedPageTable.h"
#include "VirtualAddressSpace.h"
#include <machine/openfirmware/OpenFirmware.h>
#include <machine/openfirmware/Device.h>
#include <processor/Processor.h>
#include <Log.h>
#include <panic.h>

/** A page table size, with memory hint and mask. */
struct HTABSize
{
  uint32_t memorySize; ///< The minimum size of RAM to make this size worthwhile.
  uint32_t size;   ///< The size of the HTAB.
  uint32_t mask;       ///< The mask to be entered into the SDR1 register.
};

const int g_nHtabSizes = 10;
const HTABSize g_pHtabSizes[10] =
{
  {0x800000,   0x10000, 0x000},
  {0x1000000,  0x20000, 0x001},
  {0x2000000,  0x40000, 0x003},
  {0x4000000,  0x80000, 0x007},
  {0x8000000, 0x100000, 0x00F},
  {0x10000000,0x200000, 0x01F},
  {0x20000000,0x400000, 0x03F},
  {0x40000000,0x800000, 0x07F},
  {0x80000000,0x1000000, 0x0FF},
  {0xFFFFFFFF,0x2000000, 0x1FF}
};

HashedPageTable HashedPageTable::m_Instance;

HashedPageTable &HashedPageTable::instance()
{
  return m_Instance;
}

HashedPageTable::HashedPageTable() :
  m_pHtab(0)
{
}

HashedPageTable::~HashedPageTable()
{
}

void HashedPageTable::initialise(Translation *pTranslations, size_t &nTranslations, uint32_t ramMax)
{
  // How big should our page table be?
  int selectedIndex = 0;
  for(int i = 0; i < g_nHtabSizes; i++)
  {
    if (g_pHtabSizes[i].memorySize > ramMax)
    {
      selectedIndex = i-1;
      break;
    }
  }
  if (selectedIndex == -1) selectedIndex = 0;

  // Make a mapping for our page table.
  /// \todo Use pTranslations to tell us where we can put the page table in physical RAM.
  OFDevice chosen (OpenFirmware::instance().findDevice("/chosen"));
  OFDevice mmu (chosen.getProperty("mmu"));

  OFParam ret = mmu.executeMethod("map", 4,
                    reinterpret_cast<OFParam>(0x2), // M=1
                    reinterpret_cast<OFParam>(g_pHtabSizes[selectedIndex].size),
                    reinterpret_cast<OFParam>(HTAB_VIRTUAL),
                    reinterpret_cast<OFParam>(0x200000)); // Map at 2MB. TODO use pTranslations here

  m_Size = g_pHtabSizes[selectedIndex].size;
  m_Mask = g_pHtabSizes[selectedIndex].mask;
  m_pHtab = reinterpret_cast<PTEG*> (HTAB_VIRTUAL);

  // Initialise by setting everything to zero.
  memset(reinterpret_cast<uint8_t*> (m_pHtab), 0, m_Size);

  // Add a new translation for the HTAB.
  pTranslations[nTranslations].virt = HTAB_VIRTUAL;
  pTranslations[nTranslations].phys = 0x200000;
  pTranslations[nTranslations].mode = 0x2;
  pTranslations[nTranslations].size = m_Size;
  nTranslations++;

  // For each translation, add mappings.
  for (int i = 0; i < nTranslations; i++)
  {
    Translation translation = pTranslations[i];
    for (int j = 0; j < translation.size; j += 0x1000)
    {
      // The VSID is for kernel space, which starts at 0 and extends to 15.
      // We use the top 4 bits as an index.
      addMapping(translation.virt+j, translation.phys+j, translation.mode, 0 + (translation.virt>>28));
    }
  }

  // Set up the segment registers for the kernel address space.
  Processor::setSegmentRegisters(0<<4, true, true);

  // Install the HTAB.
  /// \todo Get rid of this hardcoded 0x200000.
  uint32_t sdr1 = 0x200000 | m_Mask;
  asm volatile("mtspr 25, %0" : : "r" (sdr1));
}

void HashedPageTable::addMapping(uint32_t effectiveAddress, uint32_t physicalAddress, uint32_t mode, uint32_t vsid)
{
  uint32_t input1 = vsid&0x7FFFF; // Mask only the bottom 19 bits.
  uint32_t input2 = (effectiveAddress&0x0FFFFFFF) >> 12;
  uint32_t primaryHash = input1 ^ input2;
  uint32_t secondaryHash = ~primaryHash;

  primaryHash &= m_Mask;
  secondaryHash &= m_Mask;

  for (int i = 0; i < 8; i++)
  {
    if (m_pHtab[primaryHash].entries[i].v == 0)
    {
      // Set the PTE up.
      m_pHtab[primaryHash].entries[i].vsid = vsid;
      m_pHtab[primaryHash].entries[i].h = 0; // Primary hash
      m_pHtab[primaryHash].entries[i].api = (effectiveAddress&0x0FFFFFFF)>>22;
      m_pHtab[primaryHash].entries[i].rpn = physicalAddress>>12;
      m_pHtab[primaryHash].entries[i].wimg = 0;
      if (mode & VirtualAddressSpace::Write)
        m_pHtab[primaryHash].entries[i].pp = 0;
      else
        m_pHtab[primaryHash].entries[i].pp = 1;
      asm volatile("eieio");
      m_pHtab[primaryHash].entries[i].v = 1; // Valid.
      asm volatile("sync");
      return;
    }
  }
  for (int i = 0; i < 8; i++)
  {
    if (m_pHtab[secondaryHash].entries[i].v == 0)
    {
      // Set the PTE up.
      m_pHtab[secondaryHash].entries[i].vsid = vsid;
      m_pHtab[secondaryHash].entries[i].h = 1; // Secondary hash.
      m_pHtab[secondaryHash].entries[i].api = (effectiveAddress&0x0FFFFFFF)>>22;
      m_pHtab[secondaryHash].entries[i].rpn = physicalAddress>>12;
      m_pHtab[secondaryHash].entries[i].wimg = 0;
      if (mode & VirtualAddressSpace::Write)
        m_pHtab[secondaryHash].entries[i].pp = 0;
      else
        m_pHtab[secondaryHash].entries[i].pp = 1;
      asm volatile("eieio");
      m_pHtab[secondaryHash].entries[i].v = 1; // Valid.
      asm volatile("sync");
      return;
    }
  }

  // Else destroy the first entry in the first hash.
  /// \todo change this to random replacement.
  m_pHtab[primaryHash].entries[0].vsid = vsid;
  m_pHtab[primaryHash].entries[0].h = 0; // Primary hash
  m_pHtab[primaryHash].entries[0].api = (effectiveAddress&0x0FFFFFFF)>>22;
  m_pHtab[primaryHash].entries[0].rpn = physicalAddress>>12;
  m_pHtab[primaryHash].entries[0].wimg = 0;
  if (mode & VirtualAddressSpace::Write)
    m_pHtab[primaryHash].entries[0].pp = 0;
  else
    m_pHtab[primaryHash].entries[0].pp = 1;
  asm volatile("eieio");
  m_pHtab[primaryHash].entries[0].v = 1; // Valid.
  asm volatile("sync");
}

void HashedPageTable::removeMapping(uint32_t effectiveAddress, uint32_t vsid)
{
}

bool HashedPageTable::isMapped(uint32_t effectiveAddress, uint32_t vsid)
{
}

uint32_t HashedPageTable::getMapping(uint32_t effectiveAddress, uint32_t vsid)
{
}

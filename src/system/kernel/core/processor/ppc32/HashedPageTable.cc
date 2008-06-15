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
#include "InterruptManager.h"

extern "C" void sdr1_trampoline(uint32_t);

/** A page table size, with memory hint and mask. */
struct HTABSize
{
  uint32_t memorySize; ///< The minimum size of RAM to make this size worthwhile.
  uint32_t size;       ///< The size of the HTAB.
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
  m_pHtab(0), m_Size(0), m_Mask(0)
{
}

HashedPageTable::~HashedPageTable()
{
}

void HashedPageTable::initialise(Translations &translations, uint32_t ramMax)
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

  // Find some physical space to put our page table.
  m_Size = g_pHtabSizes[selectedIndex].size;
  uint32_t htabPhys = translations.findFreePhysicalMemory(m_Size, m_Size);
  if (htabPhys == 0)
    panic("Couldn't find anywhere to load the HTAB!");

  // Write this translation back to the Translations struct.
  translations.addTranslation(HTAB_VIRTUAL, htabPhys, m_Size, 0x10 /* M=1 */);
  WARNING("htabPhys: " << Hex << htabPhys);
  // Make a mapping for our page table.
  OFDevice chosen (OpenFirmware::instance().findDevice("/chosen"));
  OFDevice mmu (chosen.getProperty("mmu"));

  mmu.executeMethod("map", 4,
                    reinterpret_cast<OFParam>(0x2), // M=1
                    reinterpret_cast<OFParam>(g_pHtabSizes[selectedIndex].size),
                    reinterpret_cast<OFParam>(HTAB_VIRTUAL),
                    reinterpret_cast<OFParam>(htabPhys));

  // There is a minimum mask of 10 bits - the .mask parameter details how many *more* bits are masked.
  m_Mask = (g_pHtabSizes[selectedIndex].mask<<10) | 0x0000003FF;
  m_pHtab = reinterpret_cast<PTEG*> (HTAB_VIRTUAL);

  // Initialise by setting everything to zero.
  memset(reinterpret_cast<uint8_t*> (m_pHtab), 0, m_Size);
       
  // For each translation, add mappings.
  for (unsigned int i = 0; i < translations.getNumTranslations(); i++)
  {
    Translations::Translation translation = translations.getTranslation(i);
    for (unsigned int j = 0; j < translation.size; j += 0x1000)
    {
      // The VSID is for kernel space, which starts at 0 and extends to 15.
      // We use the top 4 bits as an index.
      // Convert 'mode' into a VirtualAddressSpace:: form.
      uint32_t mode = translation.mode;
      uint32_t newMode = VirtualAddressSpace::Write;
      if (mode&0x40) newMode |= VirtualAddressSpace::WriteThrough;
      if (mode&0x20) newMode |= VirtualAddressSpace::CacheDisable;
      if (mode&0x10) newMode |= VirtualAddressSpace::MemoryCoherent;
      if (mode&0x08) newMode |= VirtualAddressSpace::Guarded;

      // Special case for 0xDF000000 - that is our trampoline and needs to keep the
      // current VSID - See below.
      uint32_t vsid = 0 + (translation.virt>>28);
      if (translation.virt == 0xdf000000)
      {
        asm volatile("mfsr %0, 13" : "=r" (vsid));
        vsid &= 0x0FFFFFFFF;
      }
      addMapping(translation.virt+j, translation.phys+j, newMode, vsid);
    }
  }
  
  // Set up the segment registers for the kernel address space.

  // Install the HTAB.
  /// \todo Get rid of this hardcoded 0x200000.
  uint32_t sdr1 = htabPhys | g_pHtabSizes[selectedIndex].mask;
  PPC32InterruptManager::initialiseProcessor();

  // When we change page table, we will be running in invalid segments (there is no
  // way to guarantee that the VSIDs openfirmware has set up are the same as ours).
  // So, we have a trampoline which is linked at an address in a different segment
  // to all the kernel code (0xDF000000). We call that, which changes all segment
  // registers except SR 0xD (so it doesn't affect its own code), then changes
  // page tables. It jumps back here, and we're running in the new segment and
  // in the new page table.
  // We still need to change segment register 0xD to be 0x0000000d, however.
  sdr1_trampoline(sdr1);
  asm volatile("mtsr 13, %0" : : "r" (13));
  asm volatile("mtspr 528, %0" : : "r" (0));
  asm volatile("mtspr 530, %0" : : "r" (0));
  asm volatile("mtspr 532, %0" : : "r" (0));
  asm volatile("mtspr 534, %0" : : "r" (0));
  asm volatile("mtspr 536, %0" : : "r" (0));
  asm volatile("mtspr 538, %0" : : "r" (0));
  asm volatile("mtspr 540, %0" : : "r" (0));
  asm volatile("mtspr 542, %0" : : "r" (0));
  asm volatile("sync");
  asm volatile("isync");
}

void HashedPageTable::addMapping(uint32_t effectiveAddress, uint32_t physicalAddress, uint32_t mode, uint32_t vsid)
{
  uint32_t input1 = vsid&0x7FFFF; // Mask only the bottom 19 bits.
  uint32_t input2 = (effectiveAddress>>12)&0xffff;
  uint32_t primaryHash = input1 ^ input2;
  uint32_t secondaryHash = ~primaryHash;

  primaryHash &= m_Mask;
  secondaryHash &= m_Mask;

  int wimg = 0;
  if (mode & VirtualAddressSpace::WriteThrough)
    wimg |= 0x8;
  if (mode & VirtualAddressSpace::CacheDisable)
    wimg |= 0x4;
  if (mode & VirtualAddressSpace::MemoryCoherent)
    wimg |= 0x2;
  if (mode & VirtualAddressSpace::Guarded)
    wimg |= 0x1;

  // Assuming Ks = 0 and Kp = 1
  int pp = 0;
  if (mode & VirtualAddressSpace::KernelMode)
    pp = 0;
  else if (!(mode & VirtualAddressSpace::Write))
    pp = 1;
  else if (mode & VirtualAddressSpace::CopyOnWrite)
    pp = 3;
  else 
    pp = 2;
  pp = 0;
  for (int i = 0; i < 8; i++)
  {
    if (m_pHtab[primaryHash].entries[i].v == 0)
    {
      // Set the PTE up.
      m_pHtab[primaryHash].entries[i].vsid = vsid;
      m_pHtab[primaryHash].entries[i].h = 0; // Primary hash
      m_pHtab[primaryHash].entries[i].api = (effectiveAddress>>22)&0x3F;
      m_pHtab[primaryHash].entries[i].rpn = physicalAddress>>12;
      m_pHtab[primaryHash].entries[i].wimg = wimg;
      m_pHtab[primaryHash].entries[i].pp = pp;
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
      m_pHtab[secondaryHash].entries[i].wimg = wimg;
      m_pHtab[secondaryHash].entries[i].pp = pp;
      m_pHtab[secondaryHash].entries[i].v = 1; // Valid.
      asm volatile("sync");
      return;
    }
  }
  
  // Else destroy the last entry in the first hash.
  /// \todo change this to random replacement.
  m_pHtab[primaryHash].entries[7].v = 0;
  m_pHtab[primaryHash].entries[7].vsid = vsid;
  m_pHtab[primaryHash].entries[7].h = 0; // Primary hash
  m_pHtab[primaryHash].entries[7].api = (effectiveAddress&0x0FFFFFFF)>>22;
  m_pHtab[primaryHash].entries[7].rpn = physicalAddress>>12;
  m_pHtab[primaryHash].entries[7].wimg = wimg;
  m_pHtab[primaryHash].entries[7].pp = pp;
  m_pHtab[primaryHash].entries[7].v = 1; // Valid.
  asm volatile("sync");
}

void HashedPageTable::removeMapping(uint32_t effectiveAddress, uint32_t vsid)
{
  // Calculate the primary and secondary hashes.
  uint32_t input1 = vsid&0x7FFFF; // Mask only the bottom 19 bits.
  uint32_t input2 = (effectiveAddress>>12)&0xffff;
  uint32_t primaryHash = input1 ^ input2;
  uint32_t secondaryHash = ~primaryHash;

  primaryHash &= m_Mask;
  secondaryHash &= m_Mask;

  for (int i = 0; i < 8; i++)
  {
    // If the primary entry is valid and refers to our EA and VSID, delete it.
    if ((m_pHtab[primaryHash].entries[i].v) &&
        (m_pHtab[primaryHash].entries[i].api == ((effectiveAddress&0x0FFFFFFF)>>22)) &&
        (m_pHtab[primaryHash].entries[i].vsid == vsid))
    {
      m_pHtab[primaryHash].entries[i].v = 0;
      asm volatile("sync");
      asm volatile("tlbie %0" : : "r" (effectiveAddress));
      asm volatile("sync");
    }
  }
  for (int i = 0; i < 8; i++)
  {
    // If the secondary entry is valid and refers to our EA and VSID, delete it.
    if ((m_pHtab[secondaryHash].entries[i].v) &&
        (m_pHtab[secondaryHash].entries[i].api == ((effectiveAddress&0x0FFFFFFF)>>22)) &&
        (m_pHtab[secondaryHash].entries[i].vsid == vsid))
    {
      m_pHtab[secondaryHash].entries[i].v = 0;
      asm volatile("sync");
      asm volatile("tlbie %0" : : "r" (effectiveAddress));
      asm volatile("sync");
    }
  }
}

bool HashedPageTable::isMapped(uint32_t effectiveAddress, uint32_t vsid)
{
  // Calculate the primary and secondary hashes.
  uint32_t input1 = vsid&0x7FFFF; // Mask only the bottom 19 bits.
  uint32_t input2 = (effectiveAddress>>12)&0xffff;
  uint32_t primaryHash = input1 ^ input2;
  uint32_t secondaryHash = ~primaryHash;

  primaryHash &= m_Mask;
  secondaryHash &= m_Mask;

  for (int i = 0; i < 8; i++)
  {
    // If the primary entry is valid and refers to our EA and VSID, delete it.
    if ((m_pHtab[primaryHash].entries[i].v) &&
        (m_pHtab[primaryHash].entries[i].api == ((effectiveAddress&0x0FFFFFFF)>>22)) &&
        (m_pHtab[primaryHash].entries[i].vsid == vsid))
    {
      // Found - one exists.
      return true;
    }
  }
  for (int i = 0; i < 8; i++)
  {
    // If the secondary entry is valid and refers to our EA and VSID, delete it.
    if ((m_pHtab[secondaryHash].entries[i].v) &&
        (m_pHtab[secondaryHash].entries[i].api == ((effectiveAddress&0x0FFFFFFF)>>22)) &&
        (m_pHtab[secondaryHash].entries[i].vsid == vsid))
    {
      return true;
    }
  }
  return false;
}

uint32_t HashedPageTable::getMapping(uint32_t effectiveAddress, uint32_t vsid)
{
  // Calculate the primary and secondary hashes.
  uint32_t input1 = vsid&0x7FFFF; // Mask only the bottom 19 bits.
  uint32_t input2 = (effectiveAddress>>12)&0xffff;
  uint32_t primaryHash = input1 ^ input2;
  uint32_t secondaryHash = ~primaryHash;

  primaryHash &= m_Mask;
  secondaryHash &= m_Mask;

  for (int i = 0; i < 8; i++)
  {
    // If the primary entry is valid and refers to our EA and VSID, delete it.
    if ((m_pHtab[primaryHash].entries[i].v) &&
        (m_pHtab[primaryHash].entries[i].api == ((effectiveAddress&0x0FFFFFFF)>>22)) &&
        (m_pHtab[primaryHash].entries[i].vsid == vsid))
    {
      return m_pHtab[primaryHash].entries[i].rpn << 12;
    }
  }
  for (int i = 0; i < 8; i++)
  {
    // If the secondary entry is valid and refers to our EA and VSID, delete it.
    if ((m_pHtab[secondaryHash].entries[i].v) &&
        (m_pHtab[secondaryHash].entries[i].api == ((effectiveAddress&0x0FFFFFFF)>>22)) &&
        (m_pHtab[secondaryHash].entries[i].vsid == vsid))
    {
      return m_pHtab[secondaryHash].entries[i].rpn << 12;
    }
  }
  return 0;
}

void HashedPageTable::setIBAT(size_t n, uintptr_t virt, physical_uintptr_t phys, size_t size, uint32_t mode)
{
  uint32_t bl;
  switch(size)
  {
    case 0x20000: bl = 0x0; break; // 128KB
    case 0x40000: bl = 0x1; break; // 256KB
    case 0x80000: bl = 0x3; break; // 512KB
    case 0x100000: bl = 0x7; break; // 1MB
    case 0x200000: bl = 0xF; break; // 2MB
    case 0x400000: bl = 0x1F; break; // 4MB
    case 0x800000: bl = 0x3F; break; // 8MB
    case 0x1000000: bl = 0x7F; break; // 16MB
    case 0x2000000: bl = 0xFF; break; // 32MB
    case 0x4000000: bl = 0x1FF; break; // 64MB
    case 0x8000000: bl = 0x3FF; break; // 128MB
    case 0x10000000: bl = 0x7FF; break; // 256MB
    default: bl = 0x7F; // 16MB
  };

  BATU ibath;
  ibath.bepi = virt>>17;
  ibath.unused = 0;
  ibath.bl = bl;
  ibath.vs = (mode & VirtualAddressSpace::KernelMode) ? 1 : 0;
  ibath.vp = (mode & VirtualAddressSpace::KernelMode) ? 0 : 1;

  BATL ibatl;
  ibatl.brpn = phys>>17;
  ibatl.unused1 = 0;
  ibatl.wimg = 0;
  ibatl.unused2 = 0;
  ibatl.pp = 0x2;

  switch (n)
  {
    case 0: asm volatile("mtspr 528, %0; mtspr 529, %1" : : "r" (ibath), "r" (ibatl)); break;
    case 1: asm volatile("mtspr 530, %0; mtspr 531, %1" : : "r" (ibath), "r" (ibatl)); break;
    case 2: asm volatile("mtspr 532, %0; mtspr 533, %1" : : "r" (ibath), "r" (ibatl)); break;
    case 3: asm volatile("mtspr 534, %0; mtspr 535, %1" : : "r" (ibath), "r" (ibatl)); break;
    default: panic ("Bad index for IBAT");
  }
}

void HashedPageTable::setDBAT(size_t n, uintptr_t virt, physical_uintptr_t phys, size_t size, uint32_t mode)
{
  uint32_t bl;
  switch(size)
  {
    case 0x20000: bl = 0x0; break; // 128KB
    case 0x40000: bl = 0x1; break; // 256KB
    case 0x80000: bl = 0x3; break; // 512KB
    case 0x100000: bl = 0x7; break; // 1MB
    case 0x200000: bl = 0xF; break; // 2MB
    case 0x400000: bl = 0x1F; break; // 4MB
    case 0x800000: bl = 0x3F; break; // 8MB
    case 0x1000000: bl = 0x7F; break; // 16MB
    case 0x2000000: bl = 0xFF; break; // 32MB
    case 0x4000000: bl = 0x1FF; break; // 64MB
    case 0x8000000: bl = 0x3FF; break; // 128MB
    case 0x10000000: bl = 0x7FF; break; // 256MB
    default: bl = 0x7F; // 16MB
  };

  BATU dbath;
  dbath.bepi = virt>>17;
  dbath.unused = 0;
  dbath.bl = bl;
  dbath.vs = (mode & VirtualAddressSpace::KernelMode) ? 1 : 0;
  dbath.vp = (mode & VirtualAddressSpace::KernelMode) ? 0 : 1;

  BATL dbatl;
  dbatl.brpn = phys>>17;
  dbatl.unused1 = 0;
  dbatl.wimg = 0;
  dbatl.unused2 = 0;
  dbatl.pp = 0x2;
  
  switch (n)
  {
    case 0: asm volatile("mtspr 536, %0; mtspr 537, %1" : : "r" (dbath), "r" (dbatl)); break;
    case 1: asm volatile("mtspr 538, %0; mtspr 539, %1" : : "r" (dbath), "r" (dbatl)); break;
    case 2: asm volatile("mtspr 540, %0; mtspr 541, %1" : : "r" (dbath), "r" (dbatl)); break;
    case 3: asm volatile("mtspr 542, %0; mtspr 543, %1" : : "r" (dbath), "r" (dbatl)); break;
    default: panic ("Bad index for DBAT");
  }
}

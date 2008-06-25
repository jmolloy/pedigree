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

#include <panic.h>
#include <Log.h>
#include <utilities/utility.h>
#include <processor/Processor.h>
#include <processor/types.h>
#include <processor/PhysicalMemoryManager.h>
#include <machine/openfirmware/OpenFirmware.h>
#include <machine/openfirmware/Device.h>
#include "VirtualAddressSpace.h"
#include "HashedPageTable.h"

#define PAGE_DIRECTORY_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 12) & 0x3FF)

PPC32VirtualAddressSpace PPC32VirtualAddressSpace::m_KernelSpace;

VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace()
{
  return PPC32VirtualAddressSpace::m_KernelSpace;
}

VirtualAddressSpace *VirtualAddressSpace::create()
{
  // TODO
  //return new X86VirtualAddressSpace();
  return 0;
}

PPC32VirtualAddressSpace::PPC32VirtualAddressSpace() :
  VirtualAddressSpace(reinterpret_cast<void*> (KERNEL_VIRTUAL_HEAP)), m_Vsid(0)
{
  // Grab some VSIDs.
  m_Vsid = VsidManager::instance().obtainVsid();

  memset(reinterpret_cast<uint8_t*>(m_pPageDirectory), 0, sizeof(m_pPageDirectory));
}

PPC32VirtualAddressSpace::~PPC32VirtualAddressSpace()
{
  for (int i = 0; i < 1024; i++)
    if (m_pPageDirectory[i])
      delete m_pPageDirectory;
  // Return the VSIDs
  VsidManager::instance().returnVsid(m_Vsid);
}

bool PPC32VirtualAddressSpace::initialise(Translations &translations)
{
  // We need to map our page tables in.
  OFDevice chosen (OpenFirmware::instance().findDevice("/chosen"));
  OFDevice mmu (chosen.getProperty("mmu"));

  // Try and find some free physical memory to put the initial page tables in.
  uint32_t phys = translations.findFreePhysicalMemory(0x100000);
  if (phys == 0)
    panic("Couldn't find anywhere to load the initial page tables!");
  // We've got some physical RAM - now add a translation for it so that
  // when the hashed page table initialises it maps us in.
  /// \todo the 0x6a is temporary and wrong. Find a proper mode. (0x2 doesn't seem to work)
  translations.addTranslation(KERNEL_INITIAL_PAGE_TABLES, phys, 0x100000, 0x6a);

  OFParam ret =  mmu.executeMethod("map", 4,
                                  reinterpret_cast<OFParam>(-1),
                                  reinterpret_cast<OFParam>(0x100000),
                                  reinterpret_cast<OFParam>(KERNEL_INITIAL_PAGE_TABLES),
                                  reinterpret_cast<OFParam>(phys));
  if (ret == reinterpret_cast<OFParam>(-1))
    panic("Kernel page table mapping failed");

  // Now make sure they're all invalid.
  memset(reinterpret_cast<uint8_t*> (KERNEL_INITIAL_PAGE_TABLES),
         0,
         0x100000);
  // Map them.
  /// \todo Holy magic numbers, batman! the 0x100 is a bit hardcoded, isn't it?
  for (int i = 0; i < 0x100; i++)
    m_pPageDirectory[1023-i] = reinterpret_cast<ShadowPageTable*> (KERNEL_INITIAL_PAGE_TABLES + i*0x1000);

  return true;
}

void PPC32VirtualAddressSpace::initialRoster(Translations &translations)
{
  if (this != &m_KernelSpace)
    panic("initialRoster() called on a VA space that is not the kernel space!");

  // For every translation...
  for (unsigned int i = 0; i < translations.getNumTranslations(); i++)
  {
    Translations::Translation t = translations.getTranslation(i);
    // For every page in this translation...
    for (unsigned int j = 0; j < t.size; j += 0x1000)
    {
      void *virtualAddress = reinterpret_cast<void*> (t.virt+j);
      uint32_t physicalAddress = t.phys+j;

      uint32_t mode = t.mode;
      uint32_t newMode = VirtualAddressSpace::Write;
      if (mode&0x20) newMode |= VirtualAddressSpace::WriteThrough;
      if (mode&0x10) newMode |= VirtualAddressSpace::CacheDisable;

      // Grab the page directory entry.
      ShadowPageTable *pTable = m_pPageDirectory[PAGE_DIRECTORY_INDEX(virtualAddress)];

      // Sanity check.
      if (pTable == 0)
      {
        pTable = new ShadowPageTable;

        memset(reinterpret_cast<uint8_t*> (pTable), 0, sizeof(ShadowPageTable));
        m_pPageDirectory[PAGE_DIRECTORY_INDEX(virtualAddress)] = pTable;
      }
      // Grab the page table entry.
      pTable->entries[PAGE_TABLE_INDEX(virtualAddress)] = 
                  (physicalAddress&0xFFFFF000) | newMode;
    }
  }
}

bool PPC32VirtualAddressSpace::isAddressValid(void *virtualAddress)
{
  return true;
}

bool PPC32VirtualAddressSpace::isMapped(void *virtualAddress)
{
  // Firstly check if this is an access to kernel space.
  // If so, redirect to the kernel address space.
  uintptr_t addr = reinterpret_cast<uintptr_t> (virtualAddress);
  if (addr >= KERNEL_SPACE_START && this != &m_KernelSpace)
    return m_KernelSpace.isMapped(virtualAddress);

  // Grab the page directory entry.
  ShadowPageTable *pTable = m_pPageDirectory[PAGE_DIRECTORY_INDEX(virtualAddress)];

  // Sanity check.
  if (pTable == 0)
    return false;

  // Grab the page table entry.
  uint32_t pte = pTable->entries[PAGE_TABLE_INDEX(virtualAddress)];

  // Valid?
  if (pte == 0)
    return false;
  else
    return true;
}

bool PPC32VirtualAddressSpace::map(physical_uintptr_t physicalAddress,
                                    void *virtualAddress,
                                    size_t flags)
{
  // Firstly check if this is an access to kernel space.
  // If so, redirect to the kernel address space.
  uintptr_t addr = reinterpret_cast<uintptr_t> (virtualAddress);
  if (addr >= KERNEL_SPACE_START && this != &m_KernelSpace)
    m_KernelSpace.map(physicalAddress, virtualAddress, flags);

  // Grab the page directory entry.
  ShadowPageTable *pTable = m_pPageDirectory[PAGE_DIRECTORY_INDEX(virtualAddress)];

  // Sanity check.
  if (pTable == 0)
  {
    // New page table.
    pTable = new ShadowPageTable;
    memset(reinterpret_cast<uint8_t*> (pTable), 0, sizeof(ShadowPageTable));
    m_pPageDirectory[PAGE_DIRECTORY_INDEX(virtualAddress)] = pTable;
  }

  // Grab the page table entry.
  pTable->entries[PAGE_TABLE_INDEX(virtualAddress)] = 
    (physicalAddress&0xFFFFF000) | flags;

  // Put it in the hash table.
  HashedPageTable::instance().addMapping(addr, physicalAddress, flags, m_Vsid*8 + (addr>>28)  );

  return true;
}

void PPC32VirtualAddressSpace::getMapping(void *virtualAddress,
                                           physical_uintptr_t &physicalAddress,
                                           size_t &flags)
{
  // Firstly check if this is an access to kernel space.
  // If so, redirect to the kernel address space.
  uintptr_t addr = reinterpret_cast<uintptr_t> (virtualAddress);
  if (addr >= KERNEL_SPACE_START && this != &m_KernelSpace)
    m_KernelSpace.getMapping(virtualAddress, physicalAddress, flags);
 
  // Grab the page directory entry.
  ShadowPageTable *pTable = m_pPageDirectory[PAGE_DIRECTORY_INDEX(virtualAddress)];

  // Sanity check.
  if (pTable == 0)
    return;

  // Grab the page table entry.
  uint32_t pte = pTable->entries[PAGE_TABLE_INDEX(virtualAddress)];

  physicalAddress = pte&0xFFF;
  flags = pte&0xFFFFF000;
}

void PPC32VirtualAddressSpace::setFlags(void *virtualAddress, size_t newFlags)
{
  // Firstly check if this is an access to kernel space.
  // If so, redirect to the kernel address space.
  uintptr_t addr = reinterpret_cast<uintptr_t> (virtualAddress);
  if (addr >= KERNEL_SPACE_START && this != &m_KernelSpace)
    m_KernelSpace.setFlags(virtualAddress, newFlags);

  // Grab the page directory entry.
  ShadowPageTable *pTable = m_pPageDirectory[PAGE_DIRECTORY_INDEX(virtualAddress)];

  // Sanity check.
  if (pTable == 0)
    return;

  // Grab the page table entry.
  pTable->entries[PAGE_TABLE_INDEX(virtualAddress)] &= 0xFFFFF000;
  pTable->entries[PAGE_TABLE_INDEX(virtualAddress)] |= newFlags&0xFFF;
}

void PPC32VirtualAddressSpace::unmap(void *virtualAddress)
{
  // Firstly check if this is an access to kernel space.
  // If so, redirect to the kernel address space.
  uintptr_t addr = reinterpret_cast<uintptr_t> (virtualAddress);
  if (addr >= KERNEL_SPACE_START && this != &m_KernelSpace)
    m_KernelSpace.unmap(virtualAddress);

  // Grab the page directory entry.
  ShadowPageTable *pTable = m_pPageDirectory[PAGE_DIRECTORY_INDEX(virtualAddress)];

  // Sanity check.
  if (pTable == 0)
    return;

  // Grab the PTE.
  pTable->entries[PAGE_TABLE_INDEX(virtualAddress)] = 0;

  // Unmap from the hash table.
}

void *PPC32VirtualAddressSpace::allocateStack()
{
  uint8_t *pStackTop = new uint8_t[0x4000];
  return pStackTop+0x4000-4;
}
void PPC32VirtualAddressSpace::freeStack(void *pStack)
{
  // TODO
}

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
#include <utilities/utility.h>
#include "utils.h"
#include "VirtualAddressSpace.h"
#include <processor/PhysicalMemoryManager.h>

//
// Page Table/Directory entry flags
//
#define PAGE_PRESENT                0x01
#define PAGE_WRITE                  0x02
#define PAGE_USER                   0x04
#define PAGE_WRITE_THROUGH          0x08
#define PAGE_CACHE_DISABLE          0x10
#define PAGE_GLOBAL                 0x100
#define PAGE_NX                     0x8000000000000000

//
// Macros
//
#define PML4_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 39) & 0x1FF)
#define PAGE_DIRECTORY_POINTER_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 30) & 0x1FF)
#define PAGE_DIRECTORY_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 21) & 0x1FF)
#define PAGE_TABLE_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 12) & 0x1FF)

#define TABLE_ENTRY(table, index) (&physicalAddress(reinterpret_cast<uint64_t*>(table))[index])

#define PAGE_GET_FLAGS(x) (*x & 0x8000000000000FFF)
#define PAGE_SET_FLAGS(x, f) *x = (*x & ~0x8000000000000FFF) | f
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*x & ~0x8000000000000FFF)

// Defined in boot-standalone.s
extern void *pml4;

X64VirtualAddressSpace X64VirtualAddressSpace::m_KernelSpace(reinterpret_cast<void*>(0xFFFFFFFF90000000),
                                                             reinterpret_cast<uintptr_t>(&pml4) - 0xFFFFFFFF7FF00000);

VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace()
{
  return X64VirtualAddressSpace::m_KernelSpace;
}

static bool conditionalTableEntryAllocation(uint64_t *tableEntry, uint64_t flags)
{
  if ((*tableEntry & PAGE_PRESENT) != PAGE_PRESENT)
  {
    // Allocate a page
    PhysicalMemoryManager &PMemoryManager = PhysicalMemoryManager::instance();
    uint64_t page = PMemoryManager.allocatePage();
    if (page == 0)
      return false;

    // Map the page
    *tableEntry = page | (flags & ~(PAGE_GLOBAL | PAGE_NX));

    // Zero the page directory pointer table
    memset(physicalAddress(reinterpret_cast<void*>(page)), 0, PhysicalMemoryManager::getPageSize());
  }

  return true;
}
static bool conditionalTableEntryMapping(uint64_t *tableEntry, uint64_t physAddress, uint64_t flags)
{
  if ((*tableEntry & PAGE_PRESENT) != PAGE_PRESENT)
  {
    // Map the page
    *tableEntry = physAddress | (flags & ~(PAGE_GLOBAL | PAGE_NX));

    // Zero the page directory pointer table
    memset(physicalAddress(reinterpret_cast<void*>(physAddress)), 0, PhysicalMemoryManager::getPageSize());
    return true;
  }

  return false;
}

bool X64VirtualAddressSpace::map(physical_uintptr_t physAddress,
                                 void *virtualAddress,
                                 size_t flags)
{
  size_t Flags = toFlags(flags);
  size_t pml4Index = PML4_INDEX(virtualAddress);
  uint64_t *pml4Entry = TABLE_ENTRY(m_PhysicalPML4, pml4Index);

  // Is a page directory pointer table present?
  if (conditionalTableEntryAllocation(pml4Entry, Flags) == false)
    return false;

  size_t pageDirectoryPointerIndex = PAGE_DIRECTORY_POINTER_INDEX(virtualAddress);
  uint64_t *pageDirectoryPointerEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pml4Entry), pageDirectoryPointerIndex);

  // Is a page directory present?
  if (conditionalTableEntryAllocation(pageDirectoryPointerEntry, Flags) == false)
    return false;

  size_t pageDirectoryIndex = PAGE_DIRECTORY_INDEX(virtualAddress);
  uint64_t *pageDirectoryEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryPointerEntry), pageDirectoryIndex);

  // Is a page table present?
  if (conditionalTableEntryAllocation(pageDirectoryEntry, Flags) == false)
    return false;

  size_t pageTableIndex = PAGE_TABLE_INDEX(virtualAddress);
  uint64_t *pageTableEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryEntry), pageTableIndex);

  // Is a page already present?
  if ((*pageTableEntry & PAGE_PRESENT) == PAGE_PRESENT)
    return false;

  // Map the page
  *pageTableEntry = physAddress | Flags;

  return true;
}
bool X64VirtualAddressSpace::getMaping(void *virtualAddress,
                                       physical_uintptr_t &physAddress,
                                       size_t &flags)
{
  // TODO
  return false;
}
bool X64VirtualAddressSpace::setFlags(void *virtualAddress, size_t newFlags)
{
  // TODO
  return false;
}
bool X64VirtualAddressSpace::unmap(void *virtualAddress)
{
  // TODO
  return false;
}

bool X64VirtualAddressSpace::mapPageStructures(physical_uintptr_t physAddress,
                                               void *virtualAddress,
                                               size_t flags)
{
  size_t Flags = toFlags(flags);
  size_t pml4Index = PML4_INDEX(virtualAddress);
  uint64_t *pml4Entry = TABLE_ENTRY(m_PhysicalPML4, pml4Index);

  // Is a page directory pointer table present?
  if (conditionalTableEntryMapping(pml4Entry, physAddress, Flags) == true)
    return true;

  size_t pageDirectoryPointerIndex = PAGE_DIRECTORY_POINTER_INDEX(virtualAddress);
  uint64_t *pageDirectoryPointerEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pml4Entry), pageDirectoryPointerIndex);

  // Is a page directory present?
  if (conditionalTableEntryMapping(pageDirectoryPointerEntry, physAddress, Flags) == true)
    return true;

  size_t pageDirectoryIndex = PAGE_DIRECTORY_INDEX(virtualAddress);
  uint64_t *pageDirectoryEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryPointerEntry), pageDirectoryIndex);

  // Is a page table present?
  if (conditionalTableEntryMapping(pageDirectoryEntry, physAddress, Flags) == true)
    return true;

  size_t pageTableIndex = PAGE_TABLE_INDEX(virtualAddress);
  uint64_t *pageTableEntry = TABLE_ENTRY(PAGE_GET_PHYSICAL_ADDRESS(pageDirectoryEntry), pageTableIndex);

  // Is a page already present?
  if ((*pageTableEntry & PAGE_PRESENT) != PAGE_PRESENT)
  {
    *pageTableEntry = physAddress | flags;
    return true;
  }

  return false;
}

uint64_t X64VirtualAddressSpace::toFlags(size_t flags)
{
  uint64_t Flags = PAGE_PRESENT;
  if ((flags & KernelMode) == KernelMode)
    Flags |= PAGE_GLOBAL;
  else
    Flags |= PAGE_USER;
  if ((flags & Write) == Write)
    Flags |= PAGE_WRITE;
  if ((flags & WriteThrough) == WriteThrough)
    Flags |= PAGE_WRITE_THROUGH;
  if ((flags & CacheDisable) == CacheDisable)
    Flags |= PAGE_CACHE_DISABLE;
  if ((flags & Execute) != Execute)
    Flags |= PAGE_NX;
  return Flags;
}
size_t X64VirtualAddressSpace::fromFlags(uint64_t Flags)
{
  size_t flags = 0;
  if ((Flags & PAGE_USER) != PAGE_USER)
    flags |= KernelMode;
  if ((Flags & PAGE_WRITE) == PAGE_WRITE)
    flags |= Write;
  if ((Flags & PAGE_WRITE_THROUGH) == PAGE_WRITE_THROUGH)
    flags |= WriteThrough;
  if ((Flags & PAGE_CACHE_DISABLE) == PAGE_CACHE_DISABLE)
    flags |= CacheDisable;
  if ((Flags & PAGE_NX) != PAGE_NX)
    flags |= Execute;
  return flags;
}

X64VirtualAddressSpace::~X64VirtualAddressSpace()
{
  // TODO
}

X64VirtualAddressSpace::X64VirtualAddressSpace(void *Heap, physical_uintptr_t PhysicalPML4)
  : VirtualAddressSpace(Heap), m_PhysicalPML4(PhysicalPML4)
{
}

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
#include <processor/PhysicalMemoryManager.h>
#include "VirtualAddressSpace.h"

//
// Page Table/Directory entry flags
//
#define PAGE_PRESENT                0x01
#define PAGE_WRITE                  0x02
#define PAGE_USER                   0x04
#define PAGE_WRITE_THROUGH          0x08
#define PAGE_CACHE_DISABLE          0x10
#define PAGE_4MB                    0x80
#define PAGE_GLOBAL                 0x100
#define PAGE_SWAPPED                0x200
#define PAGE_COPY_ON_WRITE          0x400

//
// Macros
//
#define PAGE_DIRECTORY_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(x) ((reinterpret_cast<uintptr_t>(x) >> 12) & 0x3FF)

#define PAGE_DIRECTORY_ENTRY(pageDir, index) (&reinterpret_cast<uint32_t*>(pageDir)[index]);
#define PAGE_TABLE_ENTRY(VirtualPageTables, pageDirectoryIndex, index) (&reinterpret_cast<uint32_t*>(adjust_pointer(VirtualPageTables, pageDirectoryIndex * 4096))[index])

#define PAGE_GET_FLAGS(x) (*x & 0xFFF)
#define PAGE_SET_FLAGS(x, f) *x = (*x & ~0xFFF) | f
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*x & ~0xFFF)

// Defined in boot-standalone.s
extern void *pagedirectory;

X86VirtualAddressSpace X86VirtualAddressSpace::m_KernelSpace(reinterpret_cast<void*>(0xD0000000),
                                                             reinterpret_cast<uintptr_t>(&pagedirectory) - 0xC0000000,
                                                             reinterpret_cast<void*>(0xFFBFF000),
                                                             reinterpret_cast<void*>(0xFFC00000));

VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace()
{
  return X86VirtualAddressSpace::m_KernelSpace;
}

bool X86VirtualAddressSpace::isAddressValid(void *virtualAddress)
{
  return true;
}
bool X86VirtualAddressSpace::isMapped(void *virtualAddress)
{
  // TODO: This currently does not work, if we are not in this virtual address space. First
  //       I need to figure out if that is a requirement or not.

  size_t pageDirectoryIndex = PAGE_DIRECTORY_INDEX(virtualAddress);
  uint32_t *pageDirectoryEntry = PAGE_DIRECTORY_ENTRY(m_VirtualPageDirectory, pageDirectoryIndex);

  // Is a page table or 4MB page present?
  if ((*pageDirectoryEntry & PAGE_PRESENT) != PAGE_PRESENT)
    return false;

  // Is it a 4MB page?
  if ((*pageDirectoryEntry & PAGE_4MB) == PAGE_4MB)
    return true;

  size_t pageTableIndex = PAGE_TABLE_INDEX(virtualAddress);
  uint32_t *pageTableEntry = PAGE_TABLE_ENTRY(m_VirtualPageTables, pageDirectoryIndex, pageTableIndex);

  // Is a page present?
  return ((*pageTableEntry & PAGE_PRESENT) == PAGE_PRESENT);
}
bool X86VirtualAddressSpace::map(physical_uintptr_t physicalAddress,
                                 void *virtualAddress,
                                 size_t flags)
{
  // TODO: This currently does not work, if we are not in this virtual address space. First
  //       I need to figure out if that is a requirement or not.

  size_t Flags = toFlags(flags);
  size_t pageDirectoryIndex = PAGE_DIRECTORY_INDEX(virtualAddress);
  uint32_t *pageDirectoryEntry = PAGE_DIRECTORY_ENTRY(m_VirtualPageDirectory, pageDirectoryIndex);

  // Is a page table present?
  if ((*pageDirectoryEntry & PAGE_PRESENT) != PAGE_PRESENT)
  {
    // Allocate a page
    PhysicalMemoryManager &PMemoryManager = PhysicalMemoryManager::instance();
    uint32_t page = PMemoryManager.allocatePage();
    if (page == 0)
      return false;

    // Map the page
    *pageDirectoryEntry = page | (Flags & ~(PAGE_GLOBAL | PAGE_SWAPPED | PAGE_COPY_ON_WRITE));

    // Zero the page table
    memset(PAGE_TABLE_ENTRY(m_VirtualPageTables, pageDirectoryIndex, 0),
           0,
           PhysicalMemoryManager::getPageSize());
  }

  size_t pageTableIndex = PAGE_TABLE_INDEX(virtualAddress);
  uint32_t *pageTableEntry = PAGE_TABLE_ENTRY(m_VirtualPageTables, pageDirectoryIndex, pageTableIndex);

  // Is a page already present
  if ((*pageTableEntry & PAGE_PRESENT) == PAGE_PRESENT)
    return false;

  // Map the page
  *pageTableEntry = physicalAddress | Flags;

  return true;
}
void X86VirtualAddressSpace::getMapping(void *virtualAddress,
                                        physical_uintptr_t &physicalAddress,
                                        size_t &flags)
{
  // Get a pointer to the page-table entry (Also checks whether the page is actually present
  // or marked swapped out)
  uint32_t *pageTableEntry = 0;
  if (getPageTableEntry(virtualAddress, pageTableEntry) == false)
  {
    // TODO: This is a fatal error, we should panic
    return;
  }

  // Extract the physical address and the flags
  physicalAddress = PAGE_GET_PHYSICAL_ADDRESS(pageTableEntry);
  flags = fromFlags(PAGE_GET_FLAGS(pageTableEntry));
}
void X86VirtualAddressSpace::setFlags(void *virtualAddress, size_t newFlags)
{
  // Get a pointer to the page-table entry (Also checks whether the page is actually present
  // or marked swapped out)
  uint32_t *pageTableEntry = 0;
  if (getPageTableEntry(virtualAddress, pageTableEntry) == false)
  {
    // TODO: This is a fatal error, we should panic
    return;
  }

  // Set the flags
  PAGE_SET_FLAGS(pageTableEntry, toFlags(newFlags));
}
void X86VirtualAddressSpace::unmap(void *virtualAddress)
{
  // Get a pointer to the page-table entry (Also checks whether the page is actually present
  // or marked swapped out)
  uint32_t *pageTableEntry = 0;
  if (getPageTableEntry(virtualAddress, pageTableEntry) == false)
  {
    // TODO: This is a fatal error, we should panic
    return;
  }

  // Unmap the page
  *pageTableEntry = 0;
}

bool X86VirtualAddressSpace::mapPageStructures(physical_uintptr_t physicalAddress,
                                               void *virtualAddress,
                                               size_t flags)
{
  size_t pageDirectoryIndex = PAGE_DIRECTORY_INDEX(virtualAddress);
  uint32_t *pageDirectoryEntry = PAGE_DIRECTORY_ENTRY(m_VirtualPageDirectory, pageDirectoryIndex);

  // Page table present?
  if ((*pageDirectoryEntry & PAGE_PRESENT) != PAGE_PRESENT)
  {
    *pageDirectoryEntry = physicalAddress | toFlags(flags);
    return true;
  }
  else
  {
    size_t pageTableIndex = PAGE_TABLE_INDEX(virtualAddress);
    uint32_t *pageTableEntry = PAGE_TABLE_ENTRY(m_VirtualPageTables, pageDirectoryIndex, pageTableIndex);

    // Page frame present?
    if ((*pageTableEntry & PAGE_PRESENT) != PAGE_PRESENT)
    {
      *pageTableEntry = physicalAddress | toFlags(flags);
      return true;
    }
  }
  return false;
}

X86VirtualAddressSpace::~X86VirtualAddressSpace()
{
  // TODO
}

X86VirtualAddressSpace::X86VirtualAddressSpace(void *Heap,
                                               physical_uintptr_t PhysicalPageDirectory,
                                               void *VirtualPageDirectory,
                                               void *VirtualPageTables)
  : VirtualAddressSpace(Heap), m_PhysicalPageDirectory(PhysicalPageDirectory),
    m_VirtualPageDirectory(VirtualPageDirectory), m_VirtualPageTables(VirtualPageTables)
{
}

bool X86VirtualAddressSpace::getPageTableEntry(void *virtualAddress,
                                               uint32_t *&pageTableEntry)
{
  size_t pageDirectoryIndex = PAGE_DIRECTORY_INDEX(virtualAddress);
  uint32_t *pageDirectoryEntry = PAGE_DIRECTORY_ENTRY(m_VirtualPageDirectory, pageDirectoryIndex);

  // Is a page table or 4MB page present?
  if ((*pageDirectoryEntry & PAGE_PRESENT) != PAGE_PRESENT)
    return false;
  if ((*pageDirectoryEntry & PAGE_4MB) == PAGE_4MB)
    return false;

  size_t pageTableIndex = PAGE_TABLE_INDEX(virtualAddress);
  pageTableEntry = PAGE_TABLE_ENTRY(m_VirtualPageTables, pageDirectoryIndex, pageTableIndex);

  // Is a page present?
  if ((*pageTableEntry & PAGE_PRESENT) != PAGE_PRESENT ||
      (*pageTableEntry & PAGE_SWAPPED) != PAGE_SWAPPED)
    return false;

  return true;
}

uint32_t X86VirtualAddressSpace::toFlags(size_t flags)
{
  uint32_t Flags = 0;
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
  if ((flags & Swapped) == Swapped)
    Flags |= PAGE_SWAPPED;
  else
    Flags |= PAGE_PRESENT;
  if ((flags & CopyOnWrite) == CopyOnWrite)
    Flags |= PAGE_COPY_ON_WRITE;
  return Flags;
}
size_t X86VirtualAddressSpace::fromFlags(uint32_t Flags)
{
  size_t flags = Execute;
  if ((Flags & PAGE_USER) != PAGE_USER)
    flags |= KernelMode;
  if ((Flags & PAGE_WRITE) == PAGE_WRITE)
    flags |= Write;
  if ((Flags & PAGE_WRITE_THROUGH) == PAGE_WRITE_THROUGH)
    flags |= WriteThrough;
  if ((Flags & PAGE_CACHE_DISABLE) == PAGE_CACHE_DISABLE)
    flags |= CacheDisable;
  if ((Flags & PAGE_SWAPPED) == PAGE_SWAPPED)
    flags |= Swapped;
  if ((Flags & PAGE_COPY_ON_WRITE) == PAGE_COPY_ON_WRITE)
    flags |= CopyOnWrite;
  return flags;
}

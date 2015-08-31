/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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
#include <utilities/utility.h>
#include "VirtualAddressSpace.h"
#include <processor/Processor.h>
#include <processor/PhysicalMemoryManager.h>
#include <process/Scheduler.h>
#include <process/Process.h>
#include <LockGuard.h>

#include "PhysicalMemoryManager.h"

#include <sys/mman.h>
#include <errno.h>
#include <dlfcn.h>

VirtualAddressSpace *g_pCurrentlyCloning = 0;

HostedVirtualAddressSpace HostedVirtualAddressSpace::m_KernelSpace(KERNEL_VIRTUAL_HEAP, KERNEL_VIRTUAL_STACK);

typedef void *(*malloc_t)(size_t);
typedef void *(*realloc_t)(void *, size_t);
typedef void (*free_t)(void *);

#include <stdio.h>
void *__libc_malloc(size_t n)
{
  static malloc_t local = (malloc_t) dlsym(RTLD_NEXT, "malloc");
  return local(n);
}

void *__libc_realloc(void *p, size_t n)
{
  static realloc_t local = (realloc_t) dlsym(RTLD_NEXT, "realloc");
  return local(p, n);
}

void __libc_free(void *p)
{
  static free_t local = (free_t) dlsym(RTLD_NEXT, "free");
  local(p);
}

VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace()
{
  return HostedVirtualAddressSpace::m_KernelSpace;
}

VirtualAddressSpace *VirtualAddressSpace::create() {
  return new HostedVirtualAddressSpace();
}

bool HostedVirtualAddressSpace::memIsInHeap(void *pMem) {
  if (pMem < KERNEL_VIRTUAL_HEAP)
    return false;
  else if (pMem >= getEndOfHeap())
    return false;
  else
    return true;
}

void *HostedVirtualAddressSpace::getEndOfHeap() {
  return adjust_pointer(KERNEL_VIRTUAL_HEAP, KERNEL_VIRTUAL_HEAP_SIZE);
}

bool HostedVirtualAddressSpace::isAddressValid(void *virtualAddress) {
  if (reinterpret_cast<uint64_t>(virtualAddress) < 0x0008000000000000 ||
      reinterpret_cast<uint64_t>(virtualAddress) >= 0xFFF8000000000000)
    return true;
  return false;
}

bool HostedVirtualAddressSpace::isMapped(void *virtualAddress) {
  LockGuard<Spinlock> guard(m_Lock);
  int r = msync(virtualAddress, PhysicalMemoryManager::getPageSize(), MS_ASYNC);
  if (r < 0)
  {
    if (errno == ENOMEM)
    {
      return false;
    }
  }

  if(this != &getKernelAddressSpace())
  {
    bool r = getKernelAddressSpace().isMapped(virtualAddress);
    if(r)
      return r;
  }

  // Find this mapping if we can.
  for(size_t i = 0; i < m_KnownMapsSize; ++i)
  {
    if(m_pKnownMaps[i].active && m_pKnownMaps[i].vaddr == virtualAddress)
      return true;
  }

  return false;
}

bool HostedVirtualAddressSpace::map(physical_uintptr_t physAddress,
                                 void *virtualAddress, size_t flags) {
  LockGuard<Spinlock> guard(m_Lock);

  // If this should be a kernel mapping, use the kernel address space.
  if ((flags & KernelMode) && (this != &getKernelAddressSpace()))
    return getKernelAddressSpace().map(physAddress, virtualAddress, flags);

  // mmap() won't fail if the address is already mapped, but we need to.
  if(isMapped(virtualAddress))
  {
    return false;
  }

  // Map, backed onto the "physical memory" of the system.
  int prot = toFlags(flags, true);
  void *r = mmap(virtualAddress, PhysicalMemoryManager::getPageSize(), prot,
                 MAP_FIXED | MAP_SHARED,
                 HostedPhysicalMemoryManager::instance().getBackingFile(),
                 physAddress);

  if(UNLIKELY(r == MAP_FAILED))
    return false;

  // Extend list of known maps if we can't fit this one in.
  if(m_numKnownMaps == m_KnownMapsSize)
  {
    size_t oldSize = m_KnownMapsSize;
    if(m_KnownMapsSize == 0)
      m_KnownMapsSize = 1;

    m_KnownMapsSize *= 2;

    size_t newSizeBytes = sizeof(mapping_t) * m_KnownMapsSize;
    if(!m_pKnownMaps)
      m_pKnownMaps = (mapping_t *) __libc_malloc(newSizeBytes);
    else
      m_pKnownMaps = (mapping_t *) __libc_realloc(m_pKnownMaps, newSizeBytes);

    // Mark all inactive.
    for(size_t i = oldSize; i < m_KnownMapsSize; ++i)
      m_pKnownMaps[i].active = false;
  }

  // Register in the list of known mappings.
  bool bRegistered = false;
  size_t idx = m_nLastUnmap;
  for(; idx < m_KnownMapsSize; ++idx)
  {
    if(m_pKnownMaps[idx].active)
      continue;

    bRegistered = true;
    break;
  }
  if(!bRegistered)
  {
    // Try again from the beginning.
    for(idx = 0; idx < m_nLastUnmap; ++idx)
    {
      if(m_pKnownMaps[idx].active)
        continue;

      bRegistered = true;
      break;
    }
  }

  if(!bRegistered)
    panic("Fatal algorithmic error in HostedVirtualAddressSpace::map");

  m_pKnownMaps[idx].active = true;
  m_pKnownMaps[idx].vaddr = virtualAddress;
  m_pKnownMaps[idx].paddr = physAddress;
  m_pKnownMaps[idx].flags = flags;

  ++m_numKnownMaps;

  return true;
}

void HostedVirtualAddressSpace::getMapping(void *virtualAddress,
                                        physical_uintptr_t &physAddress,
                                        size_t &flags) {
  LockGuard<Spinlock> guard(m_Lock);

  // Handle kernel mappings, if needed.
  if (this != &getKernelAddressSpace())
  {
    if(getKernelAddressSpace().isMapped(virtualAddress))
    {
      getKernelAddressSpace().getMapping(virtualAddress, physAddress, flags);
      return;
    }
  }

  size_t pageSize = PhysicalMemoryManager::getPageSize();
  uintptr_t alignedVirtualAddress = reinterpret_cast<uintptr_t>(virtualAddress) & ~(pageSize - 1);
  virtualAddress = reinterpret_cast<void*>(alignedVirtualAddress);

  // Find this mapping if we can.
  for(size_t i = 0; i < m_KnownMapsSize; ++i)
  {
    if(m_pKnownMaps[i].active && m_pKnownMaps[i].vaddr == virtualAddress)
    {
      physAddress = m_pKnownMaps[i].paddr;
      flags = fromFlags(m_pKnownMaps[i].flags, true);
      return;
    }
  }

  panic("HostedVirtualAddressSpace::getMapping - function misused");
}

void HostedVirtualAddressSpace::setFlags(void *virtualAddress, size_t newFlags) {
  LockGuard<Spinlock> guard(m_Lock);

  // Check for kernel mappings.
  if (this != &getKernelAddressSpace())
  {
    if(getKernelAddressSpace().isMapped(virtualAddress))
    {
      getKernelAddressSpace().setFlags(virtualAddress, newFlags);
      return;
    }
    else if(newFlags & KernelMode)
      WARNING("setFlags called with KernelMode as a flag, page is not mapped in kernel.");
  }

  for(size_t i = 0; i < m_KnownMapsSize; ++i)
  {
    if(m_pKnownMaps[i].active && m_pKnownMaps[i].vaddr == virtualAddress)
    {
      m_pKnownMaps[i].flags = newFlags;
      break;
    }
  }

  size_t flags = toFlags(newFlags, true);
  mprotect(virtualAddress, PhysicalMemoryManager::getPageSize(), flags);
}

void HostedVirtualAddressSpace::unmap(void *virtualAddress) {
  LockGuard<Spinlock> guard(m_Lock);

  // Check for kernel mappings.
  if (this != &getKernelAddressSpace())
  {
    if(getKernelAddressSpace().isMapped(virtualAddress))
    {
      getKernelAddressSpace().unmap(virtualAddress);
      return;
    }
  }

  for(size_t i = 0; i < m_KnownMapsSize; ++i)
  {
    if(m_pKnownMaps[i].active && m_pKnownMaps[i].vaddr == virtualAddress)
    {
      m_pKnownMaps[i].active = false;
      m_nLastUnmap = i;
      break;
    }
  }

  munmap(virtualAddress, PhysicalMemoryManager::getPageSize());
}

VirtualAddressSpace *HostedVirtualAddressSpace::clone() {
  HostedVirtualAddressSpace *pNew = new HostedVirtualAddressSpace();

  {
    LockGuard<Spinlock> guard(m_Lock);

    // Copy over the known maps so the new address space can find them.
    pNew->m_pKnownMaps = (mapping_t *) __libc_malloc(m_KnownMapsSize * sizeof(mapping_t));
    memcpy(pNew->m_pKnownMaps, m_pKnownMaps, m_KnownMapsSize * sizeof(mapping_t));
    pNew->m_KnownMapsSize = m_KnownMapsSize;
    pNew->m_numKnownMaps = m_numKnownMaps;
    pNew->m_nLastUnmap = m_nLastUnmap;
  }

  return pNew;
}

void HostedVirtualAddressSpace::revertToKernelAddressSpace() {
  LockGuard<Spinlock> guard(m_Lock);

  for(size_t i = 0; i < m_KnownMapsSize; ++i)
  {
    if(m_pKnownMaps[i].active)
    {
      if(getKernelAddressSpace().isMapped(m_pKnownMaps[i].vaddr))
        continue;

      munmap(m_pKnownMaps[i].vaddr, PhysicalMemoryManager::getPageSize());
      m_pKnownMaps[i].active = false;
    }
  }
}

void *HostedVirtualAddressSpace::allocateStack() {
  size_t sz = USERSPACE_VIRTUAL_STACK_SIZE;
  if (this == &getKernelAddressSpace()) sz = KERNEL_STACK_SIZE;
  return doAllocateStack(sz);
}

void *HostedVirtualAddressSpace::allocateStack(size_t stackSz) {
  if (stackSz == 0) return allocateStack();
  return doAllocateStack(stackSz);
}

void *HostedVirtualAddressSpace::doAllocateStack(size_t sSize) {
  m_Lock.acquire();

  size_t pageSz = PhysicalMemoryManager::getPageSize();

  // Grab a new stack pointer. Use the list of freed stacks if we can, otherwise
  // adjust the internal stack pointer. Using the list of freed stacks helps
  // avoid having the virtual address creep downwards.
  void *pStack = 0;
  if (m_freeStacks.count() != 0) {
    pStack = m_freeStacks.popBack();
  } else {
    pStack = m_pStackTop;

    // Always leave one page unmapped between each stack to catch overflow.
    m_pStackTop = adjust_pointer(m_pStackTop, -(sSize + pageSz));
  }

  m_Lock.release();

  // Map the stack.
  uintptr_t firstPage = reinterpret_cast<uintptr_t>(pStack);
  NOTICE("new stack at " << firstPage);
  uintptr_t stackBottom = reinterpret_cast<uintptr_t>(pStack) - sSize;
  for (uintptr_t addr = stackBottom; addr < firstPage; addr += pageSz) {
    size_t map_flags = 0;

    physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
    map_flags = VirtualAddressSpace::Write;

    if (!map(phys, reinterpret_cast<void *>(addr), map_flags))
      WARNING("CoW map() failed in doAllocateStack");
  }

  return pStack;
}

void HostedVirtualAddressSpace::freeStack(void *pStack) {
  size_t pageSz = PhysicalMemoryManager::getPageSize();

  // Clean up the stack
  uintptr_t stackTop = reinterpret_cast<uintptr_t>(pStack);
  while (true) {
    stackTop -= pageSz;
    void *v = reinterpret_cast<void *>(stackTop);
    if (!isMapped(v)) break;  // Hit end of stack.

    size_t flags = 0;
    physical_uintptr_t phys = 0;
    getMapping(v, phys, flags);

    unmap(v);
    PhysicalMemoryManager::instance().freePage(phys);
  }

  // Add the stack to the list
  m_Lock.acquire();
  m_freeStacks.pushBack(pStack);
  m_Lock.release();
}

HostedVirtualAddressSpace::~HostedVirtualAddressSpace() {
  // TODO: Free other things, perhaps in VirtualAddressSpace
  //       We can't do this in VirtualAddressSpace destructor though!
}

HostedVirtualAddressSpace::HostedVirtualAddressSpace()
    : VirtualAddressSpace(USERSPACE_VIRTUAL_HEAP),
      m_pStackTop(USERSPACE_VIRTUAL_STACK),
      m_freeStacks(),
      m_bKernelSpace(false),
      m_Lock(false, true),
      m_pKnownMaps(0),
      m_numKnownMaps(0),
      m_nLastUnmap(0)
{
}

HostedVirtualAddressSpace::HostedVirtualAddressSpace(void *Heap, void *VirtualStack)
    : VirtualAddressSpace(Heap),
      m_pStackTop(VirtualStack),
      m_freeStacks(),
      m_bKernelSpace(true),
      m_Lock(false, true),
      m_pKnownMaps(0),
      m_numKnownMaps(0),
      m_nLastUnmap(0)
{
}

uint64_t HostedVirtualAddressSpace::toFlags(size_t flags, bool bFinal) {
  uint64_t Flags = 0;
  if(flags & Write) Flags |= PROT_WRITE;
  if(flags & Swapped)
    Flags |= PROT_NONE;
  else
    Flags |= PROT_READ;
  if (flags & Execute) Flags |= PROT_EXEC;
  return Flags;
}

size_t HostedVirtualAddressSpace::fromFlags(uint64_t Flags, bool bFinal) {
  return Flags;
}

void HostedVirtualAddressSpace::switchAddressSpace(VirtualAddressSpace &a, VirtualAddressSpace &b)
{
  HostedVirtualAddressSpace &oldSpace = static_cast<HostedVirtualAddressSpace&>(a);
  HostedVirtualAddressSpace &newSpace = static_cast<HostedVirtualAddressSpace&>(b);

  if(&oldSpace != &getKernelAddressSpace())
  {
    for(size_t i = 0; i < oldSpace.m_KnownMapsSize; ++i)
    {
      if(oldSpace.m_pKnownMaps[i].active)
      {
        if(getKernelAddressSpace().isMapped(oldSpace.m_pKnownMaps[i].vaddr))
          continue;

        munmap(oldSpace.m_pKnownMaps[i].vaddr, PhysicalMemoryManager::getPageSize());
      }
    }
  }

  for(size_t i = 0; i < newSpace.m_KnownMapsSize; ++i)
  {
    if(newSpace.m_pKnownMaps[i].active)
    {
      if(getKernelAddressSpace().isMapped(newSpace.m_pKnownMaps[i].vaddr))
        continue;

      mmap(newSpace.m_pKnownMaps[i].vaddr, PhysicalMemoryManager::getPageSize(),
           newSpace.toFlags(newSpace.m_pKnownMaps[i].flags, true), MAP_FIXED | MAP_SHARED,
           HostedPhysicalMemoryManager::instance().getBackingFile(),
           newSpace.m_pKnownMaps[i].paddr);
    }
  }
}

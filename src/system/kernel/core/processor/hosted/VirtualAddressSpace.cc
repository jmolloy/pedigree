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

VirtualAddressSpace *g_pCurrentlyCloning = 0;

HostedVirtualAddressSpace HostedVirtualAddressSpace::m_KernelSpace(KERNEL_VIRTUAL_HEAP, KERNEL_VIRTUAL_STACK);

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

  return true;
}

bool HostedVirtualAddressSpace::map(physical_uintptr_t physAddress,
                                 void *virtualAddress, size_t flags) {
  LockGuard<Spinlock> guard(m_Lock);

  // mmap() won't fail if the address is already mapped, but we need to.
  if(isMapped(virtualAddress))
  {
    return false;
  }

  // Map, backed onto the "physical memory" of the system.
  int prot = toFlags(flags, true);
  void *r = mmap(virtualAddress, PhysicalMemoryManager::getPageSize(), prot,
                 MAP_FIXED | MAP_PRIVATE,
                 HostedPhysicalMemoryManager::instance().getBackingFile(),
                 physAddress);

  return r != MAP_FAILED;
}

void HostedVirtualAddressSpace::getMapping(void *virtualAddress,
                                        physical_uintptr_t &physAddress,
                                        size_t &flags) {
  /// \todo implement
  physAddress = 0;
  flags = 0;
}

void HostedVirtualAddressSpace::setFlags(void *virtualAddress, size_t newFlags) {
  LockGuard<Spinlock> guard(m_Lock);
  mprotect(virtualAddress, PhysicalMemoryManager::getPageSize(), toFlags(newFlags, true));
}

void HostedVirtualAddressSpace::unmap(void *virtualAddress) {
  LockGuard<Spinlock> guard(m_Lock);
  munmap(virtualAddress, PhysicalMemoryManager::getPageSize());
}

VirtualAddressSpace *HostedVirtualAddressSpace::clone() {
  /// \todo implement somehow (different backing file?)
  return 0;
}

void HostedVirtualAddressSpace::revertToKernelAddressSpace() {
  /// \todo implement
}

bool HostedVirtualAddressSpace::mapPageStructures(physical_uintptr_t physAddress,
                                               void *virtualAddress,
                                               size_t flags) {
  return true;
}

bool HostedVirtualAddressSpace::mapPageStructuresAbove4GB(
    physical_uintptr_t physAddress, void *virtualAddress, size_t flags) {
  return true;
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
      m_Lock(false, true) {
}

HostedVirtualAddressSpace::HostedVirtualAddressSpace(void *Heap, void *VirtualStack)
    : VirtualAddressSpace(Heap),
      m_pStackTop(VirtualStack),
      m_freeStacks(),
      m_bKernelSpace(true),
      m_Lock(false, true) {}

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
  size_t flags = 0;
  if (!(Flags & PROT_READ)) flags |= Swapped; /// \todo probably not right
  if (Flags & PROT_WRITE) flags |= Write;
  if (Flags & PROT_EXEC) flags |= Execute;
  return flags;
}

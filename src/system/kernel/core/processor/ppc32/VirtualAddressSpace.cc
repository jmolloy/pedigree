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
#include "VirtualAddressSpace.h"

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
  VirtualAddressSpace(reinterpret_cast<void*> (KERNEL_VIRTUAL_HEAP))
{
}

PPC32VirtualAddressSpace::~PPC32VirtualAddressSpace()
{
}

bool PPC32VirtualAddressSpace::isAddressValid(void *virtualAddress)
{
  return true;
}

bool PPC32VirtualAddressSpace::isMapped(void *virtualAddress)
{

}

bool PPC32VirtualAddressSpace::map(physical_uintptr_t physicalAddress,
                                    void *virtualAddress,
                                    size_t flags)
{
}

void PPC32VirtualAddressSpace::getMapping(void *virtualAddress,
                                           physical_uintptr_t &physicalAddress,
                                           size_t &flags)
{
}

void PPC32VirtualAddressSpace::setFlags(void *virtualAddress, size_t newFlags)
{
}

void PPC32VirtualAddressSpace::unmap(void *virtualAddress)
{
}

void *PPC32VirtualAddressSpace::allocateStack()
{
  // TODO
  return 0;
}
void PPC32VirtualAddressSpace::freeStack(void *pStack)
{
  // TODO
}

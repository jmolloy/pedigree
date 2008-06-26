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

Arm926EVirtualAddressSpace Arm926EVirtualAddressSpace::m_KernelSpace;

VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace()
{
  return Arm926EVirtualAddressSpace::m_KernelSpace;
}

VirtualAddressSpace *VirtualAddressSpace::create()
{
  // TODO
  //return new X86VirtualAddressSpace();
  return 0;
}

Arm926EVirtualAddressSpace::Arm926EVirtualAddressSpace() :
  VirtualAddressSpace(reinterpret_cast<void*> (0))
{
}

Arm926EVirtualAddressSpace::~Arm926EVirtualAddressSpace()
{
}

bool Arm926EVirtualAddressSpace::initialise()
{
  return true;
}

bool Arm926EVirtualAddressSpace::isAddressValid(void *virtualAddress)
{
  return true;
}

bool Arm926EVirtualAddressSpace::isMapped(void *virtualAddress)
{
  return true;
}

bool Arm926EVirtualAddressSpace::map(physical_uintptr_t physicalAddress,
                                    void *virtualAddress,
                                    size_t flags)
{
  return false;
}

void Arm926EVirtualAddressSpace::getMapping(void *virtualAddress,
                                           physical_uintptr_t &physicalAddress,
                                           size_t &flags)
{
  physicalAddress = 0;
  flags = 0;
}

void Arm926EVirtualAddressSpace::setFlags(void *virtualAddress, size_t newFlags)
{
}

void Arm926EVirtualAddressSpace::unmap(void *virtualAddress)
{
}

void *Arm926EVirtualAddressSpace::allocateStack()
{
  // TODO
  return 0;
}
void Arm926EVirtualAddressSpace::freeStack(void *pStack)
{
  // TODO
}

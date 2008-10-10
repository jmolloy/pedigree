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

#include <machine/Device.h>
#include <processor/IoPort.h>
#include <processor/MemoryMappedIo.h>
#include <processor/PhysicalMemoryManager.h>
#include <Log.h>

/** Singleton Device instantiation. */
Device Device::m_Root;

Device::Device() : m_Addresses(), m_Children(), m_pParent(0), 
#ifdef OPENFIRMWARE
                   m_OfHandle(0),
#endif
                   m_InterruptNumber(0),
                   m_SpecificType()
{
}

Device::Device (Device *p) : m_Addresses(), m_Children(), m_pParent(0), m_InterruptNumber(p->m_InterruptNumber),
#ifdef OPENFIRMWARE
                             m_OfHandle(0),
#endif
                             m_SpecificType(p->m_SpecificType)
{
  m_pParent = p->m_pParent;
  for (unsigned int i = 0; i < p->m_Children.count(); i++)
  {
    m_Children.pushBack(p->m_Children[i]);
  }
  p->removeIoMappings();
  for (unsigned int i = 0; i < p->m_Addresses.count(); i++)
  {
    Address *pa = p->m_Addresses[i];
    Address *a = new Address(pa->m_Name, pa->m_Address, pa->m_Size, pa->m_IsIoSpace, pa->m_Padding);
    m_Addresses.pushBack(a);
  }
}

Device::~Device()
{
  for (unsigned int i = 0; i < m_Addresses.count(); i++)
  {
    delete m_Addresses[i];
  }
  for (unsigned int i = 0; i < m_Children.count(); i++)
  {
    delete m_Children[i];
  }
}

void Device::removeIoMappings()
{
  for (unsigned int i = 0; i < m_Addresses.count(); i++)
  {
    Address *pa = m_Addresses[i];
    if (pa->m_Io)
      delete pa->m_Io;
    pa->m_Io = 0;
  }
}

void Device::getName(String &str)
{
  str = "Root";
}

/** Adds a device as a child of this device. */
void Device::addChild(Device *pDevice)
{
  m_Children.pushBack(pDevice);
}
/** Retrieves the n'th child of this device. */
Device *Device::getChild(size_t n)
{
  return m_Children[n];
}
/** Retrieves the number of children of this device. */
size_t Device::getNumChildren()
{
  return m_Children.count();
}
/** Removes the n'th child of this device. */
void Device::removeChild(size_t n)
{
  unsigned int i = 0;
  for(Vector<Device*>::Iterator it = m_Children.begin();
      it != m_Children.end();
      it++, i++)
  if (i == n)
  m_Children.erase(it);
}
/** Removes the given Device from this device's child list. */
void Device::removeChild(Device *d)
{
  int i = 0;
  for(Vector<Device*>::Iterator it = m_Children.begin();
      it != m_Children.end();
      it++, i++)
  if (*it == d)
  {
    m_Children.erase(it);
    break;
  }
}

void Device::replaceChild(Device *src, Device *dest)
{
  int i = 0;
  for(Vector<Device*>::Iterator it = m_Children.begin();
      it != m_Children.end();
      it++, i++)
    if (*it == src)
    {
      *it = dest;
      break;
    }
}

Device::Address::Address(String n, uintptr_t a, size_t s, bool io, size_t pad) :
  m_Name(n), m_Address(a), m_Size(s), m_IsIoSpace(io), m_Io(0), m_Padding(pad)
{
#ifdef KERNEL_PROCESSOR_NO_PORT_IO
  // In this case, IO accesses go through MemoryMappedIo too.
  /// \todo getPageSize()
  uint32_t numPages = s / 4096;
  if (s%4096) numPages++;

  MemoryMappedIo *pIo = new MemoryMappedIo(m_Name, a%4096, pad);
  PhysicalMemoryManager &physicalMemoryManager = PhysicalMemoryManager::instance();
  if (!physicalMemoryManager.allocateRegion(*pIo,
                                       numPages,
                                       PhysicalMemoryManager::continuous | PhysicalMemoryManager::nonRamMemory |
                                       PhysicalMemoryManager::force,
                                       VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write |
                                         VirtualAddressSpace::WriteThrough | VirtualAddressSpace::CacheDisable,
                                       a))
  {
    ERROR("Device::Address - allocateRegion failed!");
  }
  m_Io = pIo;
#else
  if (m_IsIoSpace)
  {
    IoPort *pIo = new IoPort(m_Name);
    pIo->allocate(a, s);
    m_Io = pIo;
  }
  else
  {
    /// \todo getPageSize()
    uint32_t numPages = s / 4096;
    if (s%4096) numPages++;
  
    MemoryMappedIo *io = new MemoryMappedIo(m_Name, a%4096, pad);
    PhysicalMemoryManager &physicalMemoryManager = PhysicalMemoryManager::instance();
    if (!physicalMemoryManager.allocateRegion(*io,
                                        numPages,
                                        PhysicalMemoryManager::continuous | PhysicalMemoryManager::nonRamMemory | 
                                        PhysicalMemoryManager::force,
                                        VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write |
                                        VirtualAddressSpace::WriteThrough,
                                        a))
    {
      ERROR("Device::Address - allocateRegion failed!");
    }
    m_Io = io;
  }
#endif
}

Device::Address::~Address()
{
  if (m_Io)
    delete m_Io;
}

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

Device::Device() : m_Addresses(), m_Children(), m_pParent(0)
{
}
Device::Device (Device *p) : m_Addresses(), m_Children(), m_pParent(0)
{
  m_pParent = p->m_pParent;
  for (unsigned int i = 0; i < p->m_Children.count(); i++)
  {
    m_Children.pushBack(p->m_Children[i]);
  }
  for (unsigned int i = 0; i < p->m_Addresses.count(); i++)
  {
    Address *pa = p->m_Addresses[i];
    Address *a = new Address(pa->m_Name, pa->m_Address, pa->m_Size);
    m_Addresses.pushBack(a);
  }
}
Device::~Device()
{
  for (unsigned int i = 0; i < m_Addresses.count(); i++)
  {
    delete m_Addresses[i];
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
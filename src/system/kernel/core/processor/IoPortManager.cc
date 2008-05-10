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

#include <processor/Processor.h>
#include <processor/IoPortManager.h>

#if !defined(KERNEL_PROCESSOR_NO_PORT_IO)

  IoPortManager IoPortManager::m_Instance;

  bool IoPortManager::allocate(IoPort *Port,
                               io_port_t ioPort,
                               size_t size)
  {
    #if defined(ADDITIONAL_CHECKS)
      if (Processor::isInitialised() == 0)
        Processor::halt();
    #endif

    // Remove the I/O ports from the list of free I/O ports
    if (m_FreeIoPorts.allocateSpecific(ioPort, size) == false)
      return false;

    // Add information to the list of used I/O ports
    m_UsedIoPorts.pushBack(Port);
    return true;
  }

  void IoPortManager::free(IoPort *Port)
  {
    #if defined(ADDITIONAL_CHECKS)
      if (Processor::isInitialised() == 0)
        Processor::halt();
    #endif

    // Remove from the used I/O ports list
    Vector<IoPort*>::Iterator i = m_UsedIoPorts.begin();
    Vector<IoPort*>::Iterator end = m_UsedIoPorts.end();
    for (;i != end;i++)
      if ((*i) == Port)
      {
        m_UsedIoPorts.erase(i);
        break;
      }

    // Add to the free I/O ports list
    m_FreeIoPorts.free(Port->base(), Port->size());
  }

  void IoPortManager::allocateIoPortList(Vector<IoPortInfo*> &IoPorts)
  {
    for (size_t i = 0;i < m_UsedIoPorts.count();i++)
    {
      IoPortInfo *pIoPortInfo = new IoPortInfo(m_UsedIoPorts[i]->base(),
                                               m_UsedIoPorts[i]->size(),
                                               m_UsedIoPorts[i]->name());
      IoPorts.pushBack(pIoPortInfo);
    }
  }

  void IoPortManager::freeIoPortList(Vector<IoPortInfo*> &IoPorts)
  {
    while (IoPorts.count() != 0)
    {
      IoPortInfo *pIoPortInfo = IoPorts.popBack();
      delete pIoPortInfo;
    }
  }

  void IoPortManager::initialise(io_port_t ioPortBase, size_t size)
  {
    m_FreeIoPorts.free(ioPortBase, size);
  }

#endif

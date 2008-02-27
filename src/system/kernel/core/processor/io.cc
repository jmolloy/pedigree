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
#include <processor/io.h>

#ifndef KERNEL_PROCESSOR_NO_PORT_IO
  bool IoPort::allocate(io_port_t ioPort, size_t size)
  {
    // Free any allocated I/O ports
    if (m_Size != 0)
      free();

    if (IoManager::instance().allocate_io_port(ioPort, size) == true)
    {
      m_IoPort = ioPort;
      m_Size = size;
      return true;
    }
    return false;
  }

  void IoPort::free()
  {
    if (m_Size != 0)
    {
      IoManager::instance().free_io_port(m_IoPort, m_Size);

      m_Size = 0;
      m_IoPort = 0;
    }
  }
#endif

bool MemoryMappedIo::allocate(uintptr_t ioBase, size_t size)
{
  // Free any allocated I/O ports
  if (m_Size != 0)
    free();

  // TODO
  return false;
}

void MemoryMappedIo::free()
{
  if (m_Size != 0)
  {
    // TODO
  }
}

IoManager IoManager::m_Instance;

#ifndef KERNEL_PROCESSOR_NO_PORT_IO
  bool IoManager::allocate_io_port(io_port_t ioPort, size_t size)
  {
    // TODO
    return true;
  }
  void IoManager::free_io_port(io_port_t ioPort, size_t size)
  {
    // TODO
  }
#endif

void *IoManager::allocate_io_memory(void *physicalAddress, size_t size)
{
  // TODO
}
void free_io_memory(void *physicalAddress, size_t size)
{
  // TODO
}

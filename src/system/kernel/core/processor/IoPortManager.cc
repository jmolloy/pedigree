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
#if defined(DEBUGGER)
  #include <panic.h>
  #include <processor/Processor.h>
#endif
#include <processor/IoPortManager.h>

#if !defined(KERNEL_PROCESSOR_NO_PORT_IO)

  IoPortManager IoPortManager::m_Instance;
  
  bool IoPortManager::allocate(const IoPort *Port,
                               io_port_t ioPort,
                               size_t size)
  {
    #if defined(DEBUGGER)
      if (Processor::isInitialised() == 0)
        panic("IoPortManager::allocate(): function misused");
    #endif

    if (m_List.allocateSpecific(ioPort, size) == true)
    {
      // TODO Add the I/O port to another list
      return true;
    }
    return false;
  }
  void IoPortManager::free(const IoPort *Port)
  {
    #if defined(DEBUGGER)
      if (Processor::isInitialised() == 0)
        panic("IoPortManager::free(): function misused");
    #endif

    m_List.free(Port->base(), Port->size());
    // TODO Remove the I/O Port from another list
  }
  
  void IoPortManager::initialise(io_port_t ioPortBase, size_t size)
  {
    m_List.free(ioPortBase, size);
  }

#endif

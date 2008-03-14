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
#include "IoPortManager.h"

X86CommonIoPortManager X86CommonIoPortManager::m_Instance;

IoPortManager &IoPortManager::instance()
{
  return X86CommonIoPortManager::instance();
}

bool X86CommonIoPortManager::allocate(io_port_t ioPort, size_t size)
{
  // TODO m_List.allocateSpecific(ioPort, size);
  return true;
}
void X86CommonIoPortManager::free(io_port_t ioPort, size_t size)
{
  // TODO m_List.free(ioPort, size);
}

void X86CommonIoPortManager::initialise()
{
  m_List.free(0, 0x10000);
}

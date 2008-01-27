/*
 * Copyright (c) 2008 Jörg Pfähler
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
#include <io.h>

bool IoPort::allocate(IoPort_t ioPort, size_t size)
{
  // Free any allocated I/O ports
  if (m_Size != 0)
    free();
  
  if (IoPortManager::instance().allocate(ioPort, size) == true)
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
    IoPortManager::instance().free(m_IoPort, m_Size);
    
    m_Size = 0;
    m_IoPort = 0;
  }
}

IoPortManager IoPortManager::m_Instance;

bool IoPortManager::allocate(IoPort_t ioPort, size_t size)
{
  // TODO
  return true;
}
void IoPortManager::free(IoPort_t ioPort, size_t size)
{
  // TODO
}

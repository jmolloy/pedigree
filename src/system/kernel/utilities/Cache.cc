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

#include <utilities/Cache.h>
#include <utilities/utility.h>
#include <Log.h>

Cache::Cache() :
  m_CacheBuffer(0), m_CurrSize(0)
{
}

Cache::Cache(size_t nBytes)
{
  resize(nBytes);
}

Cache::~Cache()
{
  destroy();
}

bool Cache::valid()
{
  return (m_CurrSize != 0) && (m_CacheBuffer != 0);
}

void Cache::resize(size_t nBytes)
{
  destroy();
  if(nBytes)
  {
    NOTICE("Cache: resizing to " << nBytes << ".");
    m_CurrSize = nBytes;
    m_CacheBuffer = new uint8_t[m_CurrSize];
  }
}

void Cache::destroy()
{
  if(valid())
  {
    delete m_CacheBuffer;
    m_CurrSize = 0;
  }
}

size_t Cache::write(size_t offset, size_t nBytes, uintptr_t buffer)
{
  // sanity checking
  
  if(!valid())
    return 0;
  
  if(offset > m_CurrSize)
    return 0;
  
  size_t totalBytes = offset + nBytes;
  if(totalBytes > m_CurrSize)
    nBytes = m_CurrSize - offset;
  
  // actually perform the write
  memcpy(reinterpret_cast<void*> (m_CacheBuffer + offset), reinterpret_cast<void*> (buffer), nBytes);
  return nBytes;
}

size_t Cache::read(size_t offset, size_t nBytes, uintptr_t buffer)
{
  // sanity checking
  
  if(!valid())
    return 0;
  
  if(offset > m_CurrSize)
    return 0;
    
  // actually perform the read
  memcpy(reinterpret_cast<void*> (buffer), reinterpret_cast<void*> (m_CacheBuffer + offset), nBytes);
  return nBytes;
  
}

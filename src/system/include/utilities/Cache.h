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
#ifndef CACHE_H
#define CACHE_H

#include <processor/types.h>

/** Provides an abstraction of a data cache. */
class Cache
{
public:
  Cache();
  Cache(size_t nBytes);
  virtual ~Cache();
  
  /** Is the cache valid? */
  bool valid();
  
  /** Resizes the cache (WARNING: ALLOCATES this size!) */
  void resize(size_t nBytes);
  
  /** Deallocates the cache */
  void destroy();
  
  /** Writes to the cache */
  size_t write(size_t offset, size_t nBytes, uintptr_t buffer);
  
  /** Reads from the cache */
  size_t read(size_t offset, size_t nBytes, uintptr_t buffer);

private:

  uint8_t *m_CacheBuffer;
  size_t m_CurrSize;
};

#endif

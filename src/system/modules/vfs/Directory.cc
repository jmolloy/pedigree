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

#include "Directory.h"

Directory::Directory() :
  File(), m_Cache(), m_bCachePopulated(false)
{
}

Directory::Directory(String name, Time accessedTime, Time modifiedTime, Time creationTime,
           uintptr_t inode, Filesystem *pFs, size_t size, File *pParent) :
  File(name,accessedTime,modifiedTime,creationTime,inode,pFs,size,pParent),
  m_Cache(), m_bCachePopulated(false)
{
}

Directory::~Directory()
{
}

File* Directory::getChild(size_t n)
{
  if (!m_bCachePopulated)
  {
    cacheDirectoryContents();
    m_bCachePopulated = true;
  }

  int i = 0;
  for (RadixTree<File*>::Iterator it = m_Cache.begin();
       it != m_Cache.end();
       it++)
  {
    if (i == n)
      return *it;
    i++;
  }

  // Not found.
  return 0;
}

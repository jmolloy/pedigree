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

#include "VFS.h"
#include "File.h"
#include "Filesystem.h"
#include <processor/Processor.h>
#include <Log.h>

File::File() :
  m_Name(""), m_AccessedTime(0), m_ModifiedTime(0),
  m_CreationTime(0), m_Inode(0), m_pFilesystem(0), m_Size(0),
  m_bIsDirectory(false), m_bIsSymlink(false),
  m_pCachedSymlink(0), m_Cache(), m_bCachePopulated(false),
  m_pParent(0), m_bShouldDelete(true)
{
}

File::File(const File &file) :
  m_Name(file.m_Name), m_AccessedTime(file.m_AccessedTime), m_ModifiedTime(file.m_ModifiedTime),
  m_CreationTime(file.m_CreationTime), m_Inode(file.m_Inode),
  m_pFilesystem(file.m_pFilesystem), m_Size(file.m_Size),
  m_bIsDirectory(file.m_bIsDirectory), m_bIsSymlink(file.m_bIsSymlink),
  m_pCachedSymlink(file.m_pCachedSymlink),
  m_Cache(), m_bCachePopulated(false),
  m_pParent(file.m_pParent), m_bShouldDelete(file.m_bShouldDelete)
{
}

File::File(String name, Time accessedTime, Time modifiedTime, Time creationTime,
           uintptr_t inode, bool isSymlink, bool isDirectory, Filesystem *pFs, size_t size, File *pParent, bool bShouldDelete) :
  m_Name(name), m_AccessedTime(accessedTime), m_ModifiedTime(modifiedTime),
  m_CreationTime(creationTime), m_Inode(inode), m_pFilesystem(pFs),
  m_Size(size), m_bIsDirectory(isDirectory), m_bIsSymlink(isSymlink), m_pCachedSymlink(0), m_Cache(), m_bCachePopulated(false), m_pParent(pParent), m_bShouldDelete(bShouldDelete)
{
}

File::~File()
{
  if (!m_bShouldDelete)
  {
    FATAL("Deleting File with 'should delete' unset!");
  }
}

bool File::isValid()
{
  return (m_pFilesystem != 0);
}

uint64_t File::read(uint64_t location, uint64_t size, uintptr_t buffer)
{
  return m_pFilesystem->read(this, location, size, buffer);
}

uint64_t File::write(uint64_t location, uint64_t size, uintptr_t buffer)
{
  return m_pFilesystem->write(this, location, size, buffer);
}

Time File::getCreationTime()
{
  return m_CreationTime;
}

void File::setCreationTime(Time t)
{
  m_CreationTime = t;
  m_pFilesystem->fileAttributeChanged(this);
}

Time File::getAccessedTime()
{
  return m_AccessedTime;
}

void File::setAccessedTime(Time t)
{
  m_AccessedTime = t;
  m_pFilesystem->fileAttributeChanged(this);
}

Time File::getModifiedTime()
{
  return m_ModifiedTime;
}

void File::setModifiedTime(Time t)
{
  m_ModifiedTime = t;
  m_pFilesystem->fileAttributeChanged(this);
}

String File::getName()
{
  return m_Name;
}

void File::truncate()
{
  m_pFilesystem->truncate(this);
}

size_t File::getSize()
{
  return m_Size;
}

void File::setSize(size_t sz)
{
  m_Size = sz;
}

bool File::isSymlink()
{
  return m_bIsSymlink;
}

bool File::isDirectory()
{
  return m_bIsDirectory;
}

File* File::getChild(size_t n)
{
  if (!isDirectory()) return VFS::invalidFile();
  if (!m_bCachePopulated)
  {
    m_pFilesystem->cacheDirectoryContents(this);
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
  return VFS::invalidFile();
}

File *File::followLink()
{
  if (!isSymlink())
    return 0;

  if (m_pCachedSymlink)
    return m_pCachedSymlink;

  char *pBuffer = new char[1024];
  read(0ULL, static_cast<uint64_t>(getSize()), reinterpret_cast<uintptr_t>(pBuffer));
  pBuffer[getSize()] = '\0';

  return (m_pCachedSymlink = m_pFilesystem->find(String(pBuffer), m_pParent));
}

void File::cacheDirectoryContents()
{
  m_pFilesystem->cacheDirectoryContents(this);
  m_bCachePopulated = true;
}

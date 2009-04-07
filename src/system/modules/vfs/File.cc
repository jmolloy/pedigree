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
  m_pParent(0), m_nWriters(0), m_nReaders(0), m_Uid(0), m_Gid(0), m_Permissions(0)
{
}

File::File(String name, Time accessedTime, Time modifiedTime, Time creationTime,
           uintptr_t inode, Filesystem *pFs, size_t size, File *pParent) :
  m_Name(name), m_AccessedTime(accessedTime), m_ModifiedTime(modifiedTime),
  m_CreationTime(creationTime), m_Inode(inode), m_pFilesystem(pFs),
  m_Size(size), m_pParent(pParent), m_nWriters(0), m_nReaders(0), m_Uid(0), m_Gid(0), m_Permissions(0)
{
}

File::~File()
{
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
  fileAttributeChanged();
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
  fileAttributeChanged();
}

String File::getName()
{
  return m_Name;
}

size_t File::getSize()
{
  return m_Size;
}

void File::setSize(size_t sz)
{
  m_Size = sz;
}

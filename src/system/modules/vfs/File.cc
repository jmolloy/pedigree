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
  m_CreationTime(0), m_Inode(0), m_NextChild(-1), m_pFilesystem(0), m_Size(0),
  m_CustomField1(0), m_CustomField2(0)
{
}

File::File(const File &file) :
  m_Name(file.m_Name), m_AccessedTime(file.m_AccessedTime), m_ModifiedTime(file.m_ModifiedTime),
  m_CreationTime(file.m_CreationTime), m_Inode(file.m_Inode), m_NextChild(file.m_NextChild),
  m_pFilesystem(file.m_pFilesystem), m_Size(file.m_Size), m_CustomField1(file.m_CustomField1), m_CustomField2(file.m_CustomField2)
{
}

File::File(File* file) :
  m_Name(file->m_Name), m_AccessedTime(file->m_AccessedTime), m_ModifiedTime(file->m_ModifiedTime),
  m_CreationTime(file->m_CreationTime), m_Inode(file->m_Inode), m_NextChild(file->m_NextChild),
  m_pFilesystem(file->m_pFilesystem), m_Size(file->m_Size), m_CustomField1(file->m_CustomField1), m_CustomField2(file->m_CustomField2)
{
}

File &File::operator =(const File& file)
{
  m_Name = file.m_Name; m_AccessedTime = file.m_AccessedTime, m_ModifiedTime = file.m_ModifiedTime;
  m_CreationTime = file.m_CreationTime; m_Inode = file.m_Inode; m_NextChild = file.m_NextChild;
  m_pFilesystem = file.m_pFilesystem; m_Size = file.m_Size;
  m_CustomField1 = file.m_CustomField1; m_CustomField2 = file.m_CustomField2;
  
  FATAL("File operator = used");
  return *this;
}

File::File(String name, Time accessedTime, Time modifiedTime, Time creationTime,
           uintptr_t inode, bool isSymlink, bool isDirectory, Filesystem *pFs, size_t size, uint32_t custom1, uint32_t custom2) :
  m_Name(name), m_AccessedTime(accessedTime), m_ModifiedTime(modifiedTime),
  m_CreationTime(creationTime), m_Inode(inode), m_NextChild(-1), m_pFilesystem(pFs),
  m_Size(size), m_CustomField1(custom1), m_CustomField2(custom2)
{
  if (isDirectory)
    m_NextChild = 0;
  if (isSymlink)
    m_NextChild = -2;
}

File::~File()
{
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
  return (m_NextChild == -2);
}

bool File::isDirectory()
{
  return (m_NextChild >= 0);
}

File* File::firstChild()
{
  if (!isDirectory()) return VFS::invalidFile();
  m_NextChild = 1;
  return m_pFilesystem->getDirectoryChild(this, 0);
}

File* File::nextChild()
{
  if (!isDirectory()) return VFS::invalidFile();
  return m_pFilesystem->getDirectoryChild(this, m_NextChild++);
}

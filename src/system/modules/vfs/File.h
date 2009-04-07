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
#ifndef FILE_H
#define FILE_H

#include <Time.h>
#include <processor/types.h>
#include <utilities/String.h>
#include <utilities/RadixTree.h>

#define FILE_UR 0001
#define FILE_UW 0002
#define FILE_UX 0004
#define FILE_GR 0010
#define FILE_GW 0020
#define FILE_GX 0040
#define FILE_OR 0100
#define FILE_OW 0200
#define FILE_OX 0400

/** A File is a regular file - it is also the superclass of Directory, Symlink
    and Pipe. */
class File
{
  friend class Filesystem;

public:
  /** Constructor, creates an invalid file. */
  File();

  /** Copy constructors are hidden - unused! */
private:
  File(const File &file);
  File& operator =(const File&);

public:
  /** Constructor, should be called only by a Filesystem. */
  File(String name, Time accessedTime, Time modifiedTime, Time creationTime,
       uintptr_t inode, class Filesystem *pFs, size_t size, File *pParent);
  /** Destructor - doesn't do anything. */
  virtual ~File();

  /** Reads from the file. */
  virtual uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer);
  /** Writes to the file. */
  virtual uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer);

  /** Returns the time the file was created. */
  Time getCreationTime();
  /** Sets the time the file was created. */
  void setCreationTime(Time t);

  /** Returns the time the file was last accessed. */
  Time getAccessedTime();
  /** Sets the time the file was last accessed. */
  void setAccessedTime(Time t);

  /** Returns the time the file was last modified. */
  Time getModifiedTime();
  /** Sets the time the file was last modified. */
  void setModifiedTime(Time t);

  /** Returns the name of the file. */
  String getName();
  // File names cannot be changed.

  /** Delete all data from the file. */
  virtual void truncate()
  {}

  size_t getSize();
  void setSize(size_t sz);

  /** Returns true if the File is actually a symlink. */
  virtual bool isSymlink()
  {return false;}

  /** Returns true if the File is actually a directory. */
  virtual bool isDirectory()
  {return false;}

  /** Returns true if the File is actually a pipe. */
  virtual bool isPipe()
  {return false;}


  uintptr_t getInode()
  {
    return m_Inode;
  }
  void setInode(uintptr_t inode)
  {
    m_Inode = inode;
  }

  Filesystem *getFilesystem()
  {
    return m_pFilesystem;
  }
  void setFilesystem(Filesystem *pFs)
  {
    m_pFilesystem = pFs;
  }

  virtual void fileAttributeChanged()
  {}

  virtual void increaseRefCount(bool bIsWriter)
  {
    if (bIsWriter)
      m_nWriters ++;
    else
      m_nReaders ++;
  }

  virtual void decreaseRefCount(bool bIsWriter)
  {
    if (bIsWriter)
      m_nWriters --;
    else
      m_nReaders --;
  }

  void setPermissions(uint32_t perms)
  {
    m_Permissions = perms;
  }

  uint32_t getPermissions()
  {
    return m_Permissions;
  }

  void setUid(size_t uid)
  {
    m_Uid = uid;
  }
  size_t getUid()
  {
    return m_Uid;
  }

  void setGid(size_t gid)
  {
    m_Gid = gid;
  }
  size_t getGid()
  {
    return m_Gid;
  }

protected:
  String m_Name;
  Time m_AccessedTime;
  Time m_ModifiedTime;
  Time m_CreationTime;
  uintptr_t m_Inode;

  class Filesystem *m_pFilesystem;
  size_t m_Size;

  File *m_pParent;

  size_t m_nWriters, m_nReaders;

  size_t m_Uid;
  size_t m_Gid;
  uint32_t m_Permissions;
};

#endif

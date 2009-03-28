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

/** A File is a file, a directory or a symlink. */
class File
{
  friend class Filesystem;

public:
  /** Constructor, creates an invalid file. */
  File();

  /** Copy constructors are hidden - unused! */
  File(const File &file);
private:
  File& operator =(const File&);
public:
  /** Constructor, should be called only by a Filesystem. */
  File(String name, Time accessedTime, Time modifiedTime, Time creationTime,
       uintptr_t inode, bool isSymlink, bool isDirectory, class Filesystem *pFs, size_t size, File *pParent = 0, bool bShouldDelete = true);
  /** Destructor - doesn't do anything. */
  virtual ~File();

  /** Returns true if this file is "valid" - i.e. it is a real file/directory */
  bool isValid();

  /** Reads from the file. */
  uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer);
  /** Writes to the file. */
  uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer);

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
  void truncate();

  size_t getSize();
  void setSize(size_t sz);

  /** Returns true if the File is actually a symlink. */
  bool isSymlink();

  /** Returns true if the File is actually a directory. */
  bool isDirectory();

  /** Returns the n'th child of this directory, or an invalid file.
      \note This applies to directories only. Behaviour is undefined if
            this function is called on a file. */
  File* getChild(size_t n);

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

  /** Reads the contents of the file as a symbolic link. */
  File *followLink();

  /** Returns true if this file should be deleted. */
  bool shouldDelete()
  {
    return m_bShouldDelete;
  }
  void setShouldDelete(bool b)
  {
    m_bShouldDelete = b;
  }

  void cacheDirectoryContents();

private:
  String m_Name;
  Time m_AccessedTime;
  Time m_ModifiedTime;
  Time m_CreationTime;
  uintptr_t m_Inode;

  class Filesystem *m_pFilesystem;
  size_t m_Size;

  bool m_bIsDirectory;
  bool m_bIsSymlink;

  File *m_pCachedSymlink;

public:
  /** Directory contents cache. */
  RadixTree<File*> m_Cache;
  bool m_bCachePopulated;

  File *m_pParent;

private:
  bool m_bShouldDelete;
};

#endif

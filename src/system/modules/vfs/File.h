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

/** A File is a file, a directory or a symlink. */
class File
{
public:
  /** Constructor, creates an invalid file. */
  File();

  File(const File &file);

  File& operator =(const File&);

  /** Constructor, should be called only by a Filesystem. */
  File(String name, Time accessedTime, Time modifiedTime, Time creationTime,
       uintptr_t inode, bool isSymlink, bool isDirectory, class Filesystem *pFs, size_t size);
  /** Destructor - doesn't do anything. */
  ~File();

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

  /** Returns true if the File is actually a symlink. */
  bool isSymlink();

  /** Returns true if the File is actually a directory. */
  bool isDirectory();

  /** Returns the first child of this directory, or an invalid file.
      \note This applies to directories only. Behaviour is undefined if 
            this function is called on a file. */
  File firstChild();

  /** Returns the next child of this directory, or an invalid file. */
  File nextChild();

  uintptr_t getInode()
  {
    return m_Inode;
  }
  Filesystem *getFilesystem()
  {
    return m_pFilesystem;
  }

private:

  String m_Name;
  Time m_AccessedTime;
  Time m_ModifiedTime;
  Time m_CreationTime;
  uintptr_t m_Inode;
  ssize_t m_NextChild;
  class Filesystem *m_pFilesystem;
  size_t m_Size;
};

#endif

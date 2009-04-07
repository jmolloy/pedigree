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
#ifndef SYMLINK_H
#define SYMLINK_H

#include <Time.h>
#include <processor/types.h>
#include <utilities/String.h>
#include <utilities/RadixTree.h>
#include "File.h"

/** A symbolic link node. */
class Symlink : public File
{
  friend class Filesystem;

public:
  /** Eases the pain of casting, and performs a sanity check. */
  static Symlink *fromFile(File *pF)
  {
    if (!pF->isSymlink()) FATAL("Casting non-symlink File to Symlink!");
    return reinterpret_cast<Symlink*> (pF);
  }

  /** Constructor, creates an invalid file. */
  Symlink();

  /** Copy constructors are hidden - unused! */
  Symlink(const Symlink &file);
private:
  Symlink& operator =(const Symlink&);
public:
  /** Constructor, should be called only by a Filesystem. */
  Symlink(String name, Time accessedTime, Time modifiedTime, Time creationTime,
          uintptr_t inode, class Filesystem *pFs, size_t size, File *pParent);
  /** Destructor - doesn't do anything. */
  virtual ~Symlink();

  /** Returns true if the File is actually a symlink. */
  virtual bool isSymlink()
  {return true;}

  /** Reads the contents of the file as a symbolic link. */
  File *followLink();

protected:
  File *m_pCachedSymlink;
};

#endif

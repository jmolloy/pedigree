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
#ifndef PIPE_H
#define PIPE_H

#include <Time.h>
#include <processor/types.h>
#include <utilities/String.h>
#include <utilities/RadixTree.h>
#include <process/Semaphore.h>
#include "File.h"

#define PIPE_BUF_MAX 2048

/** A first-in-first-out buffer node. */
class Pipe : public File
{
  friend class Filesystem;

public:
  /** Eases the pain of casting, and performs a sanity check. */
  static Pipe *fromFile(File *pF)
  {
    if (!pF->isPipe()) FATAL("Casting non-symlink File to Pipe!");
    return reinterpret_cast<Pipe*> (pF);
  }

  /** Constructor, creates an invalid file. */
  Pipe();

  /** Copy constructors are hidden - unused! */
  Pipe(const Pipe &file);
private:
  Pipe& operator =(const Pipe&);
public:
  /** Constructor, should be called only by a Filesystem. */
  Pipe(String name, Time accessedTime, Time modifiedTime, Time creationTime,
       uintptr_t inode, class Filesystem *pFs, size_t size, File *pParent, bool bIsAnonymous = false);
  /** Destructor - doesn't do anything. */
  virtual ~Pipe();

  /** Reads from the file. */
  virtual uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer);
  /** Writes to the file. */
  virtual uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer);

  /** Returns true if the File is actually a pipe. */
  virtual bool isPipe()
  {return true;}

  virtual void increaseRefCount(bool bIsWriter);

  /** Override decreaseRefCount so we can tell when all writers have hung up
      (and also when all readers have hung up so we can die). */
  virtual void decreaseRefCount(bool bIsWriter);

protected:
  /** If we're an anonymous pipe, we should delete ourselves when all readers/writers have hung up. */
  bool m_bIsAnonymous;

  /** Have we reached EOF? */
  volatile bool m_bIsEOF;

  /** The implements needed to create a ring buffer. */
  Semaphore m_BufLen;
  Semaphore m_BufAvailable;
  uint8_t m_Buffer[PIPE_BUF_MAX];
  uintptr_t m_Front, m_Back;
};

#endif

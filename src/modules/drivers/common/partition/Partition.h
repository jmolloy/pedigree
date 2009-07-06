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
#ifndef PARTITION_H
#define PARTITION_H

#include <machine/Disk.h>
#include <utilities/StaticString.h>

/**
 * A partition is a chunk of disk.
 */
class Partition : public Disk
{
public:
  Partition(String type, uint64_t start, uint64_t length);
  virtual ~Partition();
  
  virtual void getName(String &str)
  {
    NormalStaticString str2;
    str2 += m_Type;
    str2 += " partition";
    str = str2;
  }

  virtual void dump(String &str)
  {
    LargeStaticString str2;
    str2 += m_Type;
    str2 += " partition at 0x";
    str2.append(m_Start, 16);
    str2 += "-";
    str2.append(m_Start+m_Length, 16);
    str = str2;
  }

  virtual uint64_t read(uint64_t location, uint64_t nBytes, uintptr_t buffer)
  {
    /// \todo bounds checking.
    Disk *pParent = static_cast<Disk*> (getParent());
    return pParent->read(location+m_Start, nBytes, buffer);
  }

  virtual uint64_t write(uint64_t location, uint64_t nBytes, uintptr_t buffer)
  {
    Disk *pParent = static_cast<Disk*> (getParent());
    return pParent->write(location+m_Start, nBytes, buffer);
  }

  /** Returns the first byte of the parent disk that is in this partition. */
  uint64_t getStart();

  /** Returns the length of this partition. */
  uint64_t getLength()
  {
    return m_Length;
  }

  /** Returns a string giving the type of the partition. */
  String getPartitionType()
  {
    return m_Type;
  }

private:
  String m_Type;
  uint64_t m_Start;
  uint64_t m_Length;
};

#endif

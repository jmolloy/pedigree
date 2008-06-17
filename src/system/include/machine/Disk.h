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
#ifndef MACHINE_DISK_H
#define MACHINE_DISK_H

#include <machine/Device.h>

/**
 * A disk is a random access fixed size block device.
 */
class Disk : public Device
{
public:
  Disk()
  {
  }
  Disk(Device *p) :
    Device(p)
  {
  }
  virtual ~Disk()
  {
  }

  virtual Type getType()
  {
    return Device::Disk;
  }

  virtual void getName(String &str)
  {
    str = "Generic disk";
  }

  virtual void dump(String &str)
  {
    str = "Generic disk";
  }

  /** Read nBytes from location on disk and store in 'buffer'.
   * \param location The offset from the start of the device, in bytes, to start the read.
   * \param nBytes The number of bytes to read, should be a multiple of 512.
   * \param buffer The buffer into which to place the read data. */
  virtual uint64_t read(uint64_t location, uint64_t nBytes, uintptr_t buffer)
  {
    return ~0;
  }

  /** Write nBytes from buffer in memory and store at 'location'.
   * \param location The offset from the start of the device, in bytes, to start the write.
   * \param nBytes The number of bytes to write, should be a multiple of 512.
   * \param buffer The buffer from which to read data. */
  virtual uint64_t write(uint64_t location, uint64_t nBytes, uintptr_t buffer)
  {
    return ~0;
  }
};

#endif

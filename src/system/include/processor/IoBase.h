/*
 * Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
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
#ifndef KERNEL_PROCESSOR_IOBASE_H
#define KERNEL_PROCESSOR_IOBASE_H

#include <processor/types.h>

/** @addtogroup kernelprocessor
 * @{ */

/** Interface to the hardware's I/O capabilities
 *\brief Abstrace base class for hardware I/O capabilities
 *\todo 64bit read/writes? Problem: which read/write order should be used on 32bit machines? */
class IoBase
{
  public:
    /** The destructor does nothing */
    inline virtual ~IoBase(){}

    /** Get the size of the I/O region in bytes */
    virtual size_t size() = 0;
    /** Read a byte (8bit) from the I/O Port or the memory-mapped I/O region
     *\param[in] offset offset from the I/O base port or the I/O base memory address
     *\return the byte (8bit) that have been read */
    virtual uint8_t read8(size_t offset = 0) = 0;
    /** Read two byte (16bit) from the I/O Port or the memory-mapped I/O region
     *\param[in] offset offset from the I/O base port or the I/O base memory address
     *\return the two byte (16bit) that have been read */
    virtual uint16_t read16(size_t offset = 0) = 0;
    /** Read four byte (32bit) from the I/O Port or the memory-mapped I/O region
     *\param[in] offset offset from the I/O base port or the I/O base memory address
     *\return the four byte (32bit) that have been read */
    virtual uint32_t read32(size_t offset = 0) = 0;
    /** Write a byte (8bit) to the I/O port or the memory-mapped I/O region
     *\param[in] value the value that should be written
     *\param[in] offset offset from the I/O base port or the I/O base memory address */
    virtual void write8(uint8_t value, size_t offset = 0) = 0;
    /** Write two byte (16bit) to the I/O port or the memory-mapped I/O region
     *\param[in] value the value that should be written
     *\param[in] offset offset from the I/O base port or the I/O base memory address */
    virtual void write16(uint16_t value, size_t offset = 0) = 0;
    /** Write four byte (32bit) to the I/O port or the memory-mapped I/O region
     *\param[in] value the value that should be written
     *\param[in] offset offset from the I/O base port or the I/O base memory address */
    virtual void write32(uint32_t value, size_t offset = 0) = 0;

  protected:
    /** The default constructor does nothing */
    inline IoBase(){}

  private:
    /** The copy-constructor
     *\note NOT implemented */
    IoBase(const IoBase &);
    /** The assignment operator
     *\note NOT implemented */
    IoBase &operator = (const IoBase &);
};

/** @} */

#endif

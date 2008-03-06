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
#ifndef KERNEL_PROCESSOR_MEMORYMAPPEDIO_H
#define KERNEL_PROCESSOR_MEMORYMAPPEDIO_H

#include <processor/types.h>
#include <processor/IoBase.h>

/** @ingroup kernelprocessor
 * @{ */

/** Memory-mapped I/O */
class MemoryMappedIo : public IoBase
{
  public:
    /** The default constructor */
    inline MemoryMappedIo()
      : m_IoMemory(0), m_Size(0){}
    /** The destructor frees the allocated ressources */
    inline virtual ~MemoryMappedIo(){free();}

    virtual void free();
    inline virtual size_t size();
    inline virtual uint8_t read8(size_t offset = 0);
    inline virtual uint16_t read16(size_t offset = 0);
    inline virtual uint32_t read32(size_t offset = 0);
    inline virtual void write8(uint8_t value, size_t offset = 0);
    inline virtual void write16(uint16_t value, size_t offset = 0);
    inline virtual void write32(uint32_t value, size_t offset = 0);

    /** Allocate a memory-mapped I/O range
     *\param[in] ioBase the physical base address
     *\param[in] size the size of the region in bytes
     *\return true, if successfull, false otherwise */
    bool allocate(uintptr_t ioBase, size_t size);

  private:
    /** The copy-constructor
     *\note NOT implemented */
    MemoryMappedIo(const MemoryMappedIo &);
    /** The assignment operator
     *\note NOT implemented */
    MemoryMappedIo &operator = (const MemoryMappedIo &);

    /** The base I/O memory address (virtual address) */
    volatile uint8_t *m_IoMemory;
    /** The size of the I/O memory in bytes */
    size_t m_Size;
};

/** @} */

#endif

//
// Part of the implementation
//
size_t MemoryMappedIo::size()
{
  return m_Size;
}
uint8_t MemoryMappedIo::read8(size_t offset)
{
  return *(m_IoMemory + offset);
}
uint16_t MemoryMappedIo::read16(size_t offset)
{
  return *reinterpret_cast<volatile uint16_t*>(m_IoMemory + offset);
}
uint32_t MemoryMappedIo::read32(size_t offset)
{
  return *reinterpret_cast<volatile uint32_t*>(m_IoMemory + offset);
}
void MemoryMappedIo::write8(uint8_t value, size_t offset)
{
  *(m_IoMemory + offset) = value;
}
void MemoryMappedIo::write16(uint16_t value, size_t offset)
{
  *reinterpret_cast<volatile uint16_t*>(m_IoMemory + offset) = value;
}
void MemoryMappedIo::write32(uint32_t value, size_t offset)
{
  *reinterpret_cast<volatile uint32_t*>(m_IoMemory + offset) = value;
}

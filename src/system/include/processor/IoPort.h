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

#ifndef KERNEL_PROCESSOR_IOPORT_H
#define KERNEL_PROCESSOR_IOPORT_H

#include <processor/types.h>
#include <processor/IoBase.h>
#include <utilities/String.h>

/** @addtogroup kernelprocessor
 * @{ */

#if !defined(KERNEL_PROCESSOR_NO_PORT_IO)

  /** IoPort provides access to a range of hardware I/O port
   *\brief I/O port range */
  class IoPort : public IoBase
  {
    public:
      /** The default constructor does nothing */
      inline IoPort(const char *name)
        : m_IoPort(0), m_Size(0), m_Name(name){}
      /** The destructor frees the allocated ressources */
      inline virtual ~IoPort(){free();}

      //
      // IoBase Interface
      //
      inline virtual size_t size() const;
      inline virtual uint8_t read8(size_t offset = 0);
      inline virtual uint16_t read16(size_t offset = 0);
      inline virtual uint32_t read32(size_t offset = 0);
      #if defined(BITS_64)
        inline virtual uint64_t read64(size_t offset = 0);
      #endif
      inline virtual void write8(uint8_t value, size_t offset = 0);
      inline virtual void write16(uint16_t value, size_t offset = 0);
      inline virtual void write32(uint32_t value, size_t offset = 0);
      #if defined(BITS_64)
        inline virtual void write64(uint64_t value, size_t offset = 0);
      #endif
      inline virtual operator bool() const;

      /** Get the base I/O port */
      inline io_port_t base() const;
      /** Get the name of the I/O port range
       *\return pointer to the name of the I/O port range */
      inline const char *name() const;
      /** Free an I/O port range */
      void free();
      /** Allocate an I/O port range
       *\param[in] ioPort the base I/O port
       *\param[in] size the number of successive I/O ports - 1
       *\return true, if successfull, false otherwise */
      bool allocate(io_port_t ioPort, size_t size);

    private:
      /** The copy-constructor
       *\note NOT implemented */
      IoPort(const IoPort &);
      /** The assignment operator
       *\note NOT implemented */
      IoPort &operator = (const IoPort &);

      /** The base I/O port */
      io_port_t m_IoPort;
      /** The number of successive I/O ports - 1 */
      size_t m_Size;
      /** User-visible name of this I/O port range */
      const char *m_Name;
  };

#endif

/** @} */

//
// Part of the implementation
//
#if defined(X86_COMMON)
  #include <processor/x86_common/IoPort.h>
#endif

#if !defined(KERNEL_PROCESSOR_NO_PORT_IO)
  size_t IoPort::size() const
  {
    return m_Size;
  }
  io_port_t IoPort::base() const
  {
    return m_IoPort;
  }
  IoPort::operator bool() const
  {
    return (m_Size != 0);
  }
  const char *IoPort::name() const
  {
    return m_Name;
  }
#endif

#endif

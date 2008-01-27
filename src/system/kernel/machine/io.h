/*
 * Copyright (c) 2008 Jörg Pfähler
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
#ifndef KERNEL_MACHINE_IO_H
#define KERNEL_MACHINE_IO_H

#include <machine/types.h>

/** Interface to the hardware's I/O ports.
 *\note Not synchronised */
class IoPort
{
  public:
    inline IoPort();
    /** The destructor automatically frees the I/O ports */
    inline ~IoPort();
    
    /** Allocate a range of I/O ports from the IoPortManager
     *\param[in] ioPort the base I/O port for this instance
     *\param[in] size the number of successive I/O ports to allocate - 1
     *\todo We might want to add the module name, or some equivalent here
     *\return true, if the allocation was successfull, false otherwise */
    bool allocate(IoPort_t ioPort, size_t size);
    /** Free the I/O ports */
    void free();
    
    /** Read a byte from an I/O port
     *\param[in] offset added to the base I/O port of this instance to
     *                  to get the final I/O port number
     *\return The byte read from the I/O port */
    inline uint8_t in8(size_t offset = 0);
    /** Read two byte from an I/O port
     *\param[in] offset added to the base I/O port of this instance to
     *                  to get the final I/O port number
     *\return The two byte read from the I/O port */
    inline uint16_t in16(size_t offset = 0);
    /** Read four byte from an I/O port
     *\param[in] offset added to the base I/O port of this instance to
     *                  to get the final I/O port number
     *\return The four byte read from the I/O port */
    inline uint32_t in32(size_t offset = 0);
    /** Write a byte to an I/O port
     *\param[in] value the value that should be written
     *\param[in] offset added to the base I/O port of this instance to
     *                  to get the final I/O port number */
    inline void out8(uint8_t value, size_t offset = 0);
    /** Write two byte to an I/O port
     *\param[in] value the value that should be written
     *\param[in] offset added to the base I/O port of this instance to
     *                  to get the final I/O port number */
    inline void out16(uint16_t value, size_t offset = 0);
    /** Write four byte to an I/O port
     *\param[in] value the value that should be written
     *\param[in] offset added to the base I/O port of this instance to
     *                  to get the final I/O port number */
    inline void out32(uint32_t value, size_t offset = 0);
    
  private:
    /** \note Not implemented */
    IoPort(const IoPort &);
    /** \note Not implemented */
    IoPort &operator = (const IoPort &);
    
    /*! The base I/O port */
    IoPort_t m_IoPort;
    /** The number of successive I/O ports - 1 */
    size_t m_Size;
};

/** Singleton class which manages hardware I/O port (de)allocate
 *\todo functions to enumerate the I/O ports
 *\todo locks for smp/numa systems */
class IoPortManager
{
  friend class IoPort;
  public:
    /** Get the instance of the I/O port manager */
    inline static IoPortManager &instance();
    /** Allocate a number of successive I/O ports
     *\note This is normally called from an IoPort object
     *\param[in] ioPort the I/O port number
     *\param[in] size the number of successive I/O ports - 1 to allocate
     *\todo We might want to add the module name, or some equivalent here
     *\return true, if the I/O port has been allocated successfull, false
     *        otherwise */
     bool allocate(IoPort_t ioPort, size_t size);
     /** Free a number of successive I/O ports
      *\note This is normally called from an IoPort object
      *\param[in] ioPort the I/O port number
      *\param[in] size the number of successive I/O ports - 1 to free
      *\todo do we really need the second parameter? */
     void free(IoPort_t ioPort, size_t size);
    
  private:
    /** The default constructor */
    inline IoPortManager(){}
    /** The copy-constructor
     *\note No implementation provided (IoPortManager is a singleton) */
    IoPortManager(const IoPortManager &);
    /** The assignment operator
     *\note No implementation provided (IoPortManager is a singleton) */
    IoPortManager &operator = (const IoPortManager &);
    /** The destructor */
    inline ~IoPortManager(){}
    
    /** The I/O port manager instance */
    static IoPortManager m_Instance;
};

//
// Part of the Implementation
//

IoPort::IoPort()
  : m_Size(0), m_IoPort()
{
}
IoPort::~IoPort()
{
  free();
}

IoPortManager &IoPortManager::instance()
{
  return m_Instance;
}

#ifdef X86_COMMON
  #include <machine/x86_common/io.h>
#endif

#endif

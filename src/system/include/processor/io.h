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
#ifndef KERNEL_PROCESSOR_IO_H
#define KERNEL_PROCESSOR_IO_H

#include <processor/types.h>

/** @addtogroup kernelprocessor processor-specifc kernel
 * processor-specific kernel interface
 *  @ingroup kernel
 * @{ */

/** Interface to the hardware's I/O capabilities */
class IoBase
{
  public:
    /** The destructor does nothing */
    inline virtual ~IoBase(){}

    /** Free previously allocated ressources */
    virtual void free() = 0;
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

#ifndef KERNEL_PROCESSOR_NO_PORT_IO
  /** I/O port */
  class IoPort : public IoBase
  {
    public:
      /** The default constructor */
      inline IoPort()
        : m_IoPort(0), m_Size(0){}
      /** The destructor frees the allocated ressources */
      inline virtual ~IoPort(){free();}

      virtual void free();
      inline virtual size_t size();
      inline virtual uint8_t read8(size_t offset = 0);
      inline virtual uint16_t read16(size_t offset = 0);
      inline virtual uint32_t read32(size_t offset = 0);
      inline virtual void write8(uint8_t value, size_t offset = 0);
      inline virtual void write16(uint16_t value, size_t offset = 0);
      inline virtual void write32(uint32_t value, size_t offset = 0);

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
  };
#endif

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

/** Singleton class which manages hardware I/O port and memory-mapped I/O (de)allocate
 *\todo functions to enumerate the I/O ports
 *\todo locks for smp/numa systems */
class IoManager
{
  public:
    /** Get the instance of the I/O manager */
    inline static IoManager &instance()
      {return m_Instance;}

    #ifndef KERNEL_PROCESSOR_NO_PORT_IO
      /** Allocate a number of successive I/O ports
       *\note This is normally called from an IoPort object
       *\param[in] ioPort the I/O port number
       *\param[in] size the number of successive I/O ports - 1 to allocate
       *\todo We might want to add the module name, or some equivalent here
       *\return true, if the I/O port has been allocated successfull, false
       *        otherwise */
      bool allocate_io_port(io_port_t ioPort, size_t size);
      /** Free a number of successive I/O ports
       *\note This is normally called from an IoPort object
       *\param[in] ioPort the I/O port number
       *\param[in] size the number of successive I/O ports - 1 to free
       *\todo do we really need the second parameter? */
      void free_io_port(io_port_t ioPort, size_t size);
    #endif

    // TODO
    void *allocate_io_memory(void *physicalAddress, size_t size);
    void free_io_memory(void *physicalAddress, size_t size);

  private:
    /** The default constructor */
    inline IoManager(){}
    /** The copy-constructor
     *\note No implementation provided (IoManager is a singleton) */
    IoManager(const IoManager &);
    /** The assignment operator
     *\note No implementation provided (IoManager is a singleton) */
    IoManager &operator = (const IoManager &);
    /** The destructor */
    inline ~IoManager(){}

    /** The I/O manager instance */
    static IoManager m_Instance;
};

/** @} */



//
// Part of the implementation
//
#ifdef X86_COMMON
  #include <processor/x86_common/io.h>
#endif

size_t IoPort::size()
{
  return m_Size;
}

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

#endif

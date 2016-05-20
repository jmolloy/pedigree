/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifndef KERNEL_PROCESSOR_IOPORTMANAGER_H
#define KERNEL_PROCESSOR_IOPORTMANAGER_H

#include <compiler.h>
#include <Spinlock.h>
#include <processor/types.h>
#include <processor/IoPort.h>
#include <utilities/Vector.h>
#include <utilities/RangeList.h>

/** @addtogroup kernelprocessor
 * @{ */

#if !defined(KERNEL_PROCESSOR_NO_PORT_IO)

  /** Singleton class which manages hardware I/O port (de)allocate
   *\brief Manages hardware I/O port (de)allocations */
  class IoPortManager
  {
    friend class Processor;
    public:
      /** Get the instance of the I/O manager */
      inline static IoPortManager &instance(){return m_Instance;}

      /** Structure containing information about one I/O port range. */
      struct IoPortInfo
      {
        /** Constructor initialialises all structure members
         *\param[in] IoPort base I/O port
         *\param[in] size number of successive I/O ports - 1
         *\param[in] Name user-visible description of the I/O port range */
        inline IoPortInfo(io_port_t IoPort, size_t size, const char *Name)
          : ioPort(IoPort), sIoPort(size), name(Name){}

        /** base I/O port */
        io_port_t ioPort;
        /** number of successive I/O ports - 1 */
        size_t sIoPort;
        /** User-visible description of the I/O port range */
        const char *name;
      };

      /** Allocate a number of successive I/O ports
       *\note This is normally called from an IoPort object
       *\param[in] Port pointer to the I/O port range object
       *\param[in] ioPort the I/O port number
       *\param[in] size the number of successive I/O ports - 1 to allocate
       *\return true, if the I/O port has been allocated successfull, false
       *        otherwise */
      bool allocate(IoPort *Port,
                    io_port_t ioPort,
                    size_t size);
      /** Free a number of successive I/O ports
       *\note This is normally called from an IoPort object
       *\param[in] Port pointer to the I/O port range object */
      void free(IoPort *Port);

      /** Copy the I/O port list
       *\param[in,out] IoPorts container for the copy of the I/O ports */
      void allocateIoPortList(Vector<IoPortInfo*> &IoPorts);
      /** Free the I/O port list created with allocateIoPortList.
       *\param[in,out] IoPorts container of the copy of the I/O ports */
      void freeIoPortList(Vector<IoPortInfo*> &IoPorts);

      /** Initialise the IoPortManager with an initial I/O range
       *\param[in] ioPortBase base I/O port for the range
       *\param[in] size number of successive I/O ports - 1 */
      void initialise(io_port_t ioPortBase, size_t size) INITIALISATION_ONLY;

    private:
      /** The default constructor */
      IoPortManager() INITIALISATION_ONLY;
      /** The destructor */
      virtual ~IoPortManager();
      /** The copy-constructor
       *\note No implementation provided (IoPortManager is a singleton) */
      IoPortManager(const IoPortManager &);
      /** The assignment operator
       *\note No implementation provided (IoPortManager is a singleton) */
      IoPortManager &operator = (const IoPortManager &);

      /** Lock */
      Spinlock m_Lock;

      /** The list of free I/O ports */
      RangeList<uint32_t> m_FreeIoPorts;
      /** The list of used I/O ports */
      Vector<IoPort*> m_UsedIoPorts;

      /** The IoPortManager instance */
      static IoPortManager m_Instance;
  };

#endif

/** @} */

#endif
